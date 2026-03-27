#include "pch.h"
#include "AnimationGraph.h"

#include <fstream>
#include <sstream>

// ─── 간단한 JSON 파서 헬퍼 (외부 라이브러리 없이 동작) ─────────────────────
// 프로젝트에 이미 JSON 파서가 있다면 해당 API 로 교체 가능
namespace
{
    // JSON 문자열에서 key 에 해당하는 값을 string 으로 추출
    // 단순 구현 — 중첩 오브젝트 미지원, 배열은 raw 문자열로 반환
    string JsonGetValue(const string& json, const string& key)
    {
        const string searchKey = "\"" + key + "\"";
        const size_t keyPos = json.find(searchKey);
        if (keyPos == string::npos) return "";

        size_t colon = json.find(':', keyPos + searchKey.size());
        if (colon == string::npos) return "";

        // 값 시작
        size_t start = colon + 1;
        while (start < json.size() && (json[start] == ' ' || json[start] == '\t' || json[start] == '\n' || json[start] == '\r'))
            ++start;
        if (start >= json.size()) return "";

        if (json[start] == '"')
        {
            // 문자열 값
            ++start;
            size_t end = json.find('"', start);
            return (end != string::npos) ? json.substr(start, end - start) : "";
        }
        else if (json[start] == '[')
        {
            // 배열: 대괄호 매칭으로 끝 탐색
            int depth = 0;
            size_t end = start;
            for (; end < json.size(); ++end)
            {
                if (json[end] == '[') ++depth;
                else if (json[end] == ']') { --depth; if (depth == 0) { ++end; break; } }
            }
            return json.substr(start, end - start);
        }
        else
        {
            // 숫자 / bool / null
            size_t end = start;
            while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n')
                ++end;
            return json.substr(start, end - start);
        }
    }

    // 배열 문자열에서 각 오브젝트({ ... })를 분리하여 반환
    vector<string> JsonSplitObjects(const string& arr)
    {
        vector<string> result;
        int depth = 0;
        size_t objStart = string::npos;
        for (size_t i = 0; i < arr.size(); ++i)
        {
            if (arr[i] == '{')
            {
                if (depth == 0) objStart = i;
                ++depth;
            }
            else if (arr[i] == '}')
            {
                --depth;
                if (depth == 0 && objStart != string::npos)
                {
                    result.push_back(arr.substr(objStart, i - objStart + 1));
                    objStart = string::npos;
                }
            }
        }
        return result;
    }

    AnimBlendCurve ParseCurve(const string& str)
    {
        if (str == "EaseIn")    return AnimBlendCurve::EaseIn;
        if (str == "EaseOut")   return AnimBlendCurve::EaseOut;
        if (str == "EaseInOut") return AnimBlendCurve::EaseInOut;
        return AnimBlendCurve::Linear;
    }

    // 전환 오브젝트 파싱
    AnimGraphTransition ParseTransition(const string& obj)
    {
        AnimGraphTransition t;

        const string fromStr = JsonGetValue(obj, "from");
        t.fromClip = (fromStr == "*") ? AnimGraphTransition::WILDCARD
                                       : static_cast<uint32>(stoul(fromStr.empty() ? "0" : fromStr));

        const string toStr = JsonGetValue(obj, "to");
        t.toClip = toStr.empty() ? AnimGraphTransition::WILDCARD
                                  : static_cast<uint32>(stoul(toStr));

        const string durStr = JsonGetValue(obj, "duration");
        t.duration = durStr.empty() ? 0.2f : stof(durStr);

        t.curve = ParseCurve(JsonGetValue(obj, "curve"));
        return t;
    }
}

// ─── AnimGraphLayer ──────────────────────────────────────────────────────────

float AnimGraphLayer::GetDuration(uint32 from, uint32 to) const
{
    // 1순위: 정확 일치 (from == from, to == to)
    for (const auto& t : transitions)
    {
        if (t.fromClip == from && t.toClip == to)
            return t.duration;
    }
    // 2순위: from 와일드카드 + to 일치
    for (const auto& t : transitions)
    {
        if (t.fromClip == AnimGraphTransition::WILDCARD && t.toClip == to)
            return t.duration;
    }
    // 3순위: to 와일드카드 + from 일치
    for (const auto& t : transitions)
    {
        if (t.fromClip == from && t.toClip == AnimGraphTransition::WILDCARD)
            return t.duration;
    }
    // 기본값
    return defaultDuration;
}

AnimBlendCurve AnimGraphLayer::GetCurve(uint32 from, uint32 to) const
{
    for (const auto& t : transitions)
    {
        if (t.fromClip == from && t.toClip == to)
            return t.curve;
    }
    for (const auto& t : transitions)
    {
        if (t.fromClip == AnimGraphTransition::WILDCARD && t.toClip == to)
            return t.curve;
    }
    for (const auto& t : transitions)
    {
        if (t.fromClip == from && t.toClip == AnimGraphTransition::WILDCARD)
            return t.curve;
    }
    return AnimBlendCurve::Linear;
}

// ─── AnimationGraph ──────────────────────────────────────────────────────────

bool AnimationGraph::LoadFromJson(const wstring& path)
{
    ifstream file(path);
    if (!file.is_open())
    {
        wcout << L"[AnimationGraph] JSON 파일을 열 수 없음: " << path << endl;
        return false;
    }

    const string json(
        (istreambuf_iterator<char>(file)),
        istreambuf_iterator<char>()
    );
    file.close();

    // 기본 duration 읽기
    const string defLowerStr = JsonGetValue(json, "defaultLowerDuration");
    if (!defLowerStr.empty())
        mLower.defaultDuration = stof(defLowerStr);

    const string defUpperStr = JsonGetValue(json, "defaultUpperDuration");
    if (!defUpperStr.empty())
        mUpper.defaultDuration = stof(defUpperStr);

    // Upper Enter/Exit 시간 읽기
    const string enterStr = JsonGetValue(json, "upperEnterDuration");
    if (!enterStr.empty()) mUpperEnterDuration = stof(enterStr);

    const string exitStr = JsonGetValue(json, "upperExitDuration");
    if (!exitStr.empty())  mUpperExitDuration  = stof(exitStr);

    // Lower 전환 배열 파싱
    const string lowerArr = JsonGetValue(json, "lower");
    for (const auto& obj : JsonSplitObjects(lowerArr))
        mLower.transitions.push_back(ParseTransition(obj));

    // Upper 전환 배열 파싱
    const string upperArr = JsonGetValue(json, "upper");
    for (const auto& obj : JsonSplitObjects(upperArr))
        mUpper.transitions.push_back(ParseTransition(obj));

    return true;
}

float AnimationGraph::GetLowerDuration(uint32 from, uint32 to) const
{
    return mLower.GetDuration(from, to);
}

AnimBlendCurve AnimationGraph::GetLowerCurve(uint32 from, uint32 to) const
{
    return mLower.GetCurve(from, to);
}

float AnimationGraph::GetUpperDuration(uint32 from, uint32 to) const
{
    return mUpper.GetDuration(from, to);
}

AnimBlendCurve AnimationGraph::GetUpperCurve(uint32 from, uint32 to) const
{
    return mUpper.GetCurve(from, to);
}
