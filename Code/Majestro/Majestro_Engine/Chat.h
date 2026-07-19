#pragma once

class Chat
{
public:
	// Protocol/Packet.h 의 CHAT_TEXT_CAPACITY
	static constexpr size_t kMaxInputLength = 44;
	static constexpr size_t kMaxLogSize = 40;

	// 채팅을 닫은 뒤 이 시간(초) 동안 게임 키 입력을 계속 억제
	static constexpr float kInputBlockGraceSeconds = 0.05f;

	struct Message
	{
		uint8_t playerType = 0xFF;	// PlayerType (0xFF = 알 수 없음)
		std::wstring text;
		std::chrono::steady_clock::time_point receivedAt;
	};

	static Chat& Get()
	{
		static Chat instance;
		return instance;
	}

	// 포커스
	bool IsFocused() const { return mFocused; }


	bool ShouldBlockGameInput() const
	{
		if (mFocused)
			return true;
		const float since = std::chrono::duration<float>(
			std::chrono::steady_clock::now() - mClosedAt).count();
		return since < kInputBlockGraceSeconds;
	}

	void Open()
	{
		mFocused = true;
		mInput.clear();
		mComposition.clear();
	}

	// ESC
	void CancelInput()
	{
		mFocused = false;
		mInput.clear();
		mComposition.clear();
		mClosedAt = std::chrono::steady_clock::now();	// 유예 시작 시각 기록
	}

	// Enter
	void CommitInput()
	{
		if (!mInput.empty())
			mPendingSends.push_back(mInput);
		CancelInput();
	}

	// 텍스트 입력
	void AppendChar(wchar_t ch)
	{
		if (mInput.size() < kMaxInputLength)
			mInput.push_back(ch);
	}

	void Backspace()
	{
		if (!mInput.empty())
			mInput.pop_back();
	}

	void SetComposition(const std::wstring& comp) { mComposition = comp; }
	void ClearComposition() { mComposition.clear(); }

	const std::wstring& GetInput() const { return mInput; }
	const std::wstring& GetComposition() const { return mComposition; }

	// 전송 큐
	template<typename Fn>
	void ConsumePendingSends(Fn&& fn)
	{
		for (const std::wstring& text : mPendingSends)
			fn(text);
		mPendingSends.clear();
	}

	// 수신 로그
	void AddMessage(uint8_t playerType, const std::wstring& text)
	{
		Message msg;
		msg.playerType = playerType;
		msg.text = text.substr(0, kMaxInputLength);
		msg.receivedAt = std::chrono::steady_clock::now();
		mLog.push_back(std::move(msg));

		while (mLog.size() > kMaxLogSize)
			mLog.pop_front();
	}

	const std::deque<Message>& GetLog() const { return mLog; }

	// 비포커스 상태 표시 알파
	float GetMessageAlpha(const Message& msg) const
	{
		constexpr float kLifetimeSeconds = 8.0f;
		constexpr float kFadeSeconds = 1.5f;

		const float age = std::chrono::duration<float>(
			std::chrono::steady_clock::now() - msg.receivedAt).count();

		if (age >= kLifetimeSeconds)
			return 0.f;
		if (age <= kLifetimeSeconds - kFadeSeconds)
			return 1.f;
		return (kLifetimeSeconds - age) / kFadeSeconds;
	}

	bool IsCaretVisible() const
	{
		const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		return (ms / 530) % 2 == 0;
	}

private:
	Chat() = default;

	bool mFocused = false;
	std::chrono::steady_clock::time_point mClosedAt{};	// 닫은 시각

	std::wstring mInput;
	std::wstring mComposition;			
	std::vector<std::wstring> mPendingSends;
	std::deque<Message> mLog;
};
