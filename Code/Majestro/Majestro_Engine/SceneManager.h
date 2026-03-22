#pragma once
class Scene;
class SceneManager;

enum class SceneCommandType
{
	None,
	LoadScene,
	LoadingThenScene,

};

enum {
	MAX_LAYER = 32
};

// 씬 전환 명령 구조체
struct SceneCommand
{
	SceneCommandType  type = SceneCommandType::None;		
	SceneId           targetScene = SceneId::MainMenu;		

	// LoadingThenScene일 때만 사용
	wstring           loadingMessage;
	SceneCommandType visualType = SceneCommandType::None;

	bool IsValid() const { return type != SceneCommandType::None; }
};

class SceneCommandBuilder
{
public:
	// 즉시 씬 전환
	static SceneCommand LoadScene(SceneId id)
	{
		SceneCommand cmd;
		cmd.type = SceneCommandType::LoadScene;
		cmd.targetScene = id;
		return cmd;
	}

	// 로딩씬 거쳐서 전환
	static SceneCommand LoadSceneWithLoading(
		SceneId id,
		const wstring& message)
	{
		SceneCommand cmd;
		cmd.type = SceneCommandType::LoadingThenScene;
		cmd.targetScene = id;
		cmd.loadingMessage = message;
		return cmd;
	}


private:
	SceneCommand mCmd;
};

class SceneCommandProcessor
{
public:
	explicit SceneCommandProcessor(SceneManager* owner): mOwner(owner) {}

	// 명령 예약 (덮어쓰기 - 마지막 명령만 유효)
	void Submit(SceneCommand cmd);

	// 매 프레임 StartRender() 이전에 호출
	// true 반환 = 씬 전환이 실행됨
	bool Process();

	bool HasPending() const { return mPending.IsValid(); }

private:
	void ExecuteLoadScene(const SceneCommand& cmd);

	void ExecuteLoadingThenScene(const SceneCommand& cmd);

	SceneManager* mOwner;
	SceneCommand  mPending;
};



class SceneManager
{
public: // Scene 기본
	void Initialize();
	
	void Update(float deltaTime);
	void Render();

public: // Scene 전환
	void FactoryScene();	// 전체 씬을 생성하는 함수 (예: 로비 씬, 게임 씬 등)

	void RequestScene(SceneCommand cmd)
	{
		mProcessor.Submit(std::move(cmd));
	}

	void RequestScene(SceneId id)	// 씬 ID로 간단히 전환 요청하는 함수 (예: 게임 모드에서 씬 전환할 때 등)
	{
		mProcessor.Submit(SceneCommandBuilder::LoadScene(id));
	}

	void RequestSceneWithLoading(SceneId id, const wstring& message)
	{
		mProcessor.Submit(SceneCommandBuilder::LoadSceneWithLoading(id, message));
	}

	void ProcessCommands()	// 전환 명령을 처리하는 함수 (매 프레임 StartRender() 이전에 호출)
	{
		mProcessor.Process();
	}

	void CheckGameModeSceneRequest();	// 씬 전환을 처리하는 함수 (예: 로비에서 게임으로, 게임에서 로비로 등)
	void LoadScene(SceneId id);	// 즉시 씬을 로드하는 함수 (예: 게임 시작 시 첫 씬 로드)

public:	// utility
	shared_ptr<Scene> GetActiveScene() { return mActiveScene; }
	const wstring& GetLoadingMessage() const { return mLoadingMessage; }
	
	void SetLayerName(uint8 index, const wstring& name);
	void SetLoadingMessage(const wstring& loadingMessage);

	const wstring& IndexToLayerName(uint8 index) { return mLayerNames[index]; }
	uint8 LayerNameToIndex(const wstring& name);
private:
	std::array< shared_ptr<Scene>, (uint8)SceneId::End> mGameScenes;	// 씬 ID에 따른 씬 포인터 배열
	shared_ptr<Scene> mActiveScene;
	SceneCommand	mSceneCommand;	// 씬 전환 명령 구조체
	SceneCommandProcessor          mProcessor{ this };

	// 로딩 관련 (렌더링에서 참조)
	wstring           mLoadingMessage;
	SceneCommandType mLoadingVisualType = SceneCommandType::None;

	//layer를 양쪽에서 찾을 수 있게 매핑
	array<wstring, MAX_LAYER> mLayerNames;
	map<wstring, uint8> mLayerIndex;
private:
	friend class SceneCommandProcessor;  // Processor만 LoadScene 접근 가능
};