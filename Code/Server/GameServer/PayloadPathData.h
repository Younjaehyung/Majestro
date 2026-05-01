#pragma once
#include "Object.h"
#include <string>
#include <vector>

struct PayloadPathPoint
{
	float distance = 0.f;
	Vec3 position = Vec3::Zero;
	Vec3 forward = Vec3::Backward;
	Vec3 up = Vec3::Up;
};

struct PayloadCheckpoint
{
	std::string name;
	float distance = 0.f;
	float radius = 0.f;
	float addTimeSeconds = 0.f;
	std::string attackerSpawnGroup;
	std::string defenderSpawnGroup;
};

struct PayloadStopPoint
{
	std::string name;
	float distance = 0.f;
	float waitSeconds = 0.f;
	std::string resumeEvent;
};

struct PayloadSpeedZone
{
	std::string name;
	float startDistance = 0.f;
	float endDistance = 0.f;
	float speedMultiplier = 1.f;
};

struct PayloadEventPoint
{
	std::string name;
	float distance = 0.f;
	std::string eventId;
	bool fireOnce = true;
	std::string direction = "forward";
};

struct PayloadPathSample
{
	float distance = 0.f;
	Vec3 position = Vec3::Zero;
	Vec3 forward = Vec3::Backward;
	Vec3 up = Vec3::Up;
	Matrix worldMatrix = Matrix::Identity;
};

class PayloadPathData : public Object
{
public:
	PayloadPathData() : Object(OBJECT_TYPE::PayloadPath) {}
	bool IsValid() const { return points.size() >= 2 && length > 0.f; }
	bool LoadFromJsonFile(const std::wstring& path);
	bool Evaluate(float distance, PayloadPathSample& outSample) const;
	PayloadPathSample EvaluateClamped(float distance) const;
	float AdvanceDistance(float currentDistance, float deltaTime, float baseSpeed) const;
	float GetSpeedMultiplier(float distance) const;
	bool DidPassDistance(float previousDistance, float currentDistance, float targetDistance) const;
	void CollectPassedEvents(float previousDistance, float currentDistance, std::vector<const PayloadEventPoint*>& outEvents) const;

	float GetLength() const { return length; }
private:
	std::string name;
	int version = 1;
	std::string unit = "cm";
	std::string coordinateSpace = "local";
	float sampleInterval = 0.f;
	bool closedLoop = false;
	float length = 0.f;

	std::vector<PayloadPathPoint> points;
	std::vector<PayloadCheckpoint> checkpoints;
	std::vector<PayloadStopPoint> stopPoints;
	std::vector<PayloadSpeedZone> speedZones;
	std::vector<PayloadEventPoint> eventPoints;

};
