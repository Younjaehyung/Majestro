#include "pch.h"
#include "PayloadPathData.h"
#include "JsonUtils.h"

bool PayloadPathData::LoadFromJsonFile(const std::wstring& path)
{
	std::ifstream ifs(ws2s(path));
	if (!ifs.is_open())
		throw std::runtime_error("Failed to open payload path json: " + ws2s(path));

	json root;
	ifs >> root;

	name = GetOptionalString(root, "name");
	version = root.contains("version") ? root["version"].get<int>() : 1;
	unit = GetOptionalString(root, "unit", "cm");
	coordinateSpace = GetOptionalString(root, "coordinateSpace", "local");
	sampleInterval = GetOptionalFloat(root, "sampleInterval", 0.f);
	closedLoop = GetOptionalBool(root, "closedLoop", false);

	const float unitScale = (unit == "m") ? 100.f : 1.f;
	length = GetOptionalFloat(root, "length", 0.f) * unitScale;

	points.clear();
	checkpoints.clear();
	stopPoints.clear();
	speedZones.clear();
	eventPoints.clear();

	const auto& jsonPoints = RequireJson(root, "points");
	if (!jsonPoints.is_array())
		throw std::runtime_error("Payload path JSON 'points' must be an array");

	points.reserve(jsonPoints.size());
	for (const auto& pointJson : jsonPoints)
	{
		PayloadPathPoint point{};
		point.distance = GetOptionalFloat(pointJson, "distance", 0.f) * unitScale;
		point.position = ConvertUeVectorToDx(ParseVec3ArrayOrObject(RequireJson(pointJson, "position"), unitScale));
		point.forward = SafeNormalize(ConvertUeVectorToDx(ParseVec3ArrayOrObject(RequireJson(pointJson, "forward"), 1.f)), Vec3::Backward);
		point.up = SafeNormalize(ConvertUeVectorToDx(ParseVec3ArrayOrObject(RequireJson(pointJson, "up"), 1.f)), Vec3::Up);
		points.push_back(point);
	}

	std::sort(points.begin(), points.end(), [](const PayloadPathPoint& lhs, const PayloadPathPoint& rhs)
		{
			return lhs.distance < rhs.distance;
		});

	if (length <= 0.f && !points.empty())
		length = points.back().distance;

	if (root.contains("checkpoints") && root["checkpoints"].is_array())
	{
		for (const auto& checkpointJson : root["checkpoints"])
		{
			PayloadCheckpoint checkpoint{};
			checkpoint.name = GetOptionalString(checkpointJson, "name");
			checkpoint.distance = GetOptionalFloat(checkpointJson, "distance", 0.f) * unitScale;
			checkpoint.radius = GetOptionalFloat(checkpointJson, "radius", 0.f) * unitScale;
			checkpoint.addTimeSeconds = GetOptionalFloat(checkpointJson, "addTimeSeconds", 0.f);
			checkpoint.attackerSpawnGroup = GetOptionalString(checkpointJson, "attackerSpawnGroup");
			checkpoint.defenderSpawnGroup = GetOptionalString(checkpointJson, "defenderSpawnGroup");
			checkpoints.push_back(checkpoint);
		}
	}

	if (root.contains("stopPoints") && root["stopPoints"].is_array())
	{
		for (const auto& stopJson : root["stopPoints"])
		{
			PayloadStopPoint stop{};
			stop.name = GetOptionalString(stopJson, "name");
			stop.distance = GetOptionalFloat(stopJson, "distance", 0.f) * unitScale;
			stop.waitSeconds = GetOptionalFloat(stopJson, "waitSeconds", 0.f);
			stop.resumeEvent = GetOptionalString(stopJson, "resumeEvent");
			stopPoints.push_back(stop);
		}
	}

	if (root.contains("speedZones") && root["speedZones"].is_array())
	{
		for (const auto& zoneJson : root["speedZones"])
		{
			PayloadSpeedZone zone{};
			zone.name = GetOptionalString(zoneJson, "name");
			zone.startDistance = GetOptionalFloat(zoneJson, "startDistance", 0.f) * unitScale;
			zone.endDistance = GetOptionalFloat(zoneJson, "endDistance", 0.f) * unitScale;
			zone.speedMultiplier = GetOptionalFloat(zoneJson, "speedMultiplier", 1.f);
			if (zone.endDistance < zone.startDistance)
				std::swap(zone.startDistance, zone.endDistance);
			speedZones.push_back(zone);
		}
	}

	if (root.contains("eventPoints") && root["eventPoints"].is_array())
	{
		for (const auto& eventJson : root["eventPoints"])
		{
			PayloadEventPoint eventPoint{};
			eventPoint.name = GetOptionalString(eventJson, "name");
			eventPoint.distance = GetOptionalFloat(eventJson, "distance", 0.f) * unitScale;
			eventPoint.eventId = GetOptionalString(eventJson, "eventId");
			eventPoint.fireOnce = GetOptionalBool(eventJson, "fireOnce", true);
			eventPoint.direction = GetOptionalString(eventJson, "direction", "forward");
			eventPoints.push_back(eventPoint);
		}
	}

	return IsValid();
}

bool PayloadPathData::Evaluate(float distance, PayloadPathSample& outSample) const
{
	if (points.empty())
		return false;

	distance = NormalizeDistanceForPath(distance, length, closedLoop);

	if (points.size() == 1 || distance <= points.front().distance)
	{
		const PayloadPathPoint& point = points.front();
		outSample.distance = point.distance;
		outSample.position = point.position;
		outSample.forward = point.forward;
		outSample.up = point.up;
		outSample.worldMatrix = BuildPayloadWorldMatrix(outSample.position, outSample.forward, outSample.up);
		return true;
	}

	if (distance >= points.back().distance)
	{
		const PayloadPathPoint& point = points.back();
		outSample.distance = point.distance;
		outSample.position = point.position;
		outSample.forward = point.forward;
		outSample.up = point.up;
		outSample.worldMatrix = BuildPayloadWorldMatrix(outSample.position, outSample.forward, outSample.up);
		return true;
	}

	auto upperIt = std::lower_bound(points.begin(), points.end(), distance, [](const PayloadPathPoint& point, float value)
		{
			return point.distance < value;
		});

	const PayloadPathPoint& next = *upperIt;
	const PayloadPathPoint& prev = *(upperIt - 1);
	const float segmentLength = next.distance - prev.distance;
	const float t = (segmentLength > 0.0001f) ? ((distance - prev.distance) / segmentLength) : 0.f;

	
	outSample.distance = distance;
	outSample.position = Vec3::Lerp(prev.position, next.position, t);
	outSample.forward = SafeNormalize(Vec3::Lerp(prev.forward, next.forward, t), prev.forward);
	outSample.up = SafeNormalize(Vec3::Lerp(prev.up, next.up, t), prev.up);
	outSample.worldMatrix = BuildPayloadWorldMatrix(outSample.position, outSample.forward, outSample.up);
	return true;
}

PayloadPathSample PayloadPathData::EvaluateClamped(float distance) const
{
	PayloadPathSample sample{};
	Evaluate(distance, sample);
	return sample;
}

float PayloadPathData::AdvanceDistance(float currentDistance, float deltaTime, float baseSpeed) const
{
	const float speedMultiplier = GetSpeedMultiplier(currentDistance);
	const float nextDistance = currentDistance + baseSpeed * speedMultiplier * deltaTime;
	return NormalizeDistanceForPath(nextDistance, length, closedLoop);
}

float PayloadPathData::GetSpeedMultiplier(float distance) const
{
	distance = NormalizeDistanceForPath(distance, length, closedLoop);

	float multiplier = 1.f;
	for (const PayloadSpeedZone& zone : speedZones)
	{
		if (distance >= zone.startDistance && distance <= zone.endDistance)
			multiplier *= zone.speedMultiplier;
	}

	return multiplier;
}

bool PayloadPathData::DidPassDistance(float previousDistance, float currentDistance, float targetDistance) const
{
	if (currentDistance >= previousDistance)
		return previousDistance < targetDistance && currentDistance >= targetDistance;

	return previousDistance > targetDistance && currentDistance <= targetDistance;
}

void PayloadPathData::CollectPassedEvents(float previousDistance, float currentDistance, std::vector<const PayloadEventPoint*>& outEvents) const
{
	for (const PayloadEventPoint& eventPoint : eventPoints)
	{
		if (eventPoint.direction == "forward" && currentDistance < previousDistance)
			continue;

		if (eventPoint.direction == "backward" && currentDistance > previousDistance)
			continue;

		if (DidPassDistance(previousDistance, currentDistance, eventPoint.distance))
			outEvents.push_back(&eventPoint);
	}
}
