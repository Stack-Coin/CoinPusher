// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/CPPressAnyKeyWidget.h"
#include "Datatables/CPCutSceneData.h"
#include "CPVideoCutSceneUIWidget.generated.h"

class UImage;
class UTextBlock;
class UDataTable;
class UMediaPlayer;
class UMediaTexture;

/**
 *  CutSceneDataTable(Row Struct: FCPCutSceneData)의 행들을 Sequence_Index 오름차순으로 순서대로
 *  재생하는 컷신 화면. UCPPressAnyKeyWidget을 상속해 아무 키보드/마우스/게임패드 입력이든 감지되면
 *  다음 Sequence_Index로 진행한다(부모의 자동 위젯 전환은 ScheduleSwitchToNextWidget을 빈
 *  오버라이드로 막아둠 - 매 입력마다 자동 전환되면 시퀀스 진행 로직과 충돌하기 때문).
 *
 *  각 행의 Video/TextFont/FontSize/Text/ScriptBoxImage 중 비어있는 필드는 새로운(비어있지 않은)
 *  값이 나오는 행을 만나기 전까지 직전 값을 그대로 유지한다(필드 단위로 독립적으로 유지 - 예:
 *  Text만 있고 Video가 없는 행은 영상은 그대로 두고 텍스트만 갱신). FontSize는 TextFont(폰트
 *  자체)와 별개로 취급되어, 폰트는 그대로 두고 크기만 바꾸는 행을 만들 수 있다. bItalic은 bool이라
 *  "지정 안 함" 상태가 없으므로 유지되지 않고 매 시퀀스마다 그 값 그대로 적용된다.
 *
 *  DataTable의 모든 행을 다 재생했으면(마지막 행에서 추가 입력) NextLevelName으로
 *  UGameplayStatics::OpenLevel()을 호출해 다음 레벨을 불러온다.
 */
UCLASS(abstract)
class CP_API UCPVideoCutSceneUIWidget : public UCPPressAnyKeyWidget
{
	GENERATED_BODY()

protected:

	/** 컷신 시퀀스 데이터 테이블 (Row Struct는 FCPCutSceneData여야 함) */
	UPROPERTY(EditAnywhere, Category="Video CutScene")
	TObjectPtr<UDataTable> CutSceneDataTable;

	/** 영상을 재생하는 화면. Brush의 ResourceObject를 WBP에서 이미 MediaTexture로 지정해뒀다면
	 *  비워둬도 되고(선택 사항), MediaTexture도 함께 지정해두면 NativeConstruct가 대신 적용해준다 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> VideoImage;

	/** 대사/설명 텍스트 뒤에 깔리는 스크립트창 배경 이미지 (선택 사항). Row의 ScriptBoxImage로
	 *  매 시퀀스마다 갱신됨(ScriptBoxImage가 비어있으면 직전 이미지 유지) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ScriptBackgroundImage;

	/** 현재 시퀀스의 Text/TextFont를 보여줄 텍스트 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScriptText;

	/** "아무 버튼이나 눌러 계속" 안내용 버튼 이미지 (선택 사항) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ButtonImage;

	/** Video를 실제로 재생시키는 Media Player 에셋 (Content Browser에서 미리 만들어 지정해야 함) */
	UPROPERTY(EditAnywhere, Category="Video CutScene")
	TObjectPtr<UMediaPlayer> MediaPlayer;

	/** MediaPlayer의 출력을 받는 Media Texture 에셋. VideoImage와 함께 지정해두면 NativeConstruct가
	 *  VideoImage의 Brush에 자동으로 적용한다(WBP에서 이미 Brush를 지정해뒀다면 지정하지 않아도 됨) */
	UPROPERTY(EditAnywhere, Category="Video CutScene")
	TObjectPtr<UMediaTexture> MediaTexture;

	/** Row.bItalic이 true인 시퀀스에서 ScriptText에 적용할 기울기 강도(FSlateFontInfo::SkewAmount,
	 *  -5~5). 실제 이탤릭 서체가 아니라 폰트를 비스듬히 기울이는 방식(Faux Italic)이라 폰트 자체를
	 *  바꾸지 않고도 적용 가능 */
	UPROPERTY(EditAnywhere, Category="Video CutScene", meta = (ClampMin = -5, ClampMax = 5))
	float ItalicSkewAmount = 0.1f;

	/** 모든 시퀀스를 다 재생했을 때(HandleSequenceExhausted) OpenLevel로 이동할 다음 레벨 */
	UPROPERTY(EditAnywhere, Category="Video CutScene")
	FName NextLevelName;

	/** CutSceneDataTable의 행들을 Sequence_Index 오름차순으로 정렬해 캐싱해둔 배열 (NativeConstruct에서 채움) */
	TArray<FCPCutSceneData> SortedCutScenes;

	/** 현재 재생 중인 SortedCutScenes의 인덱스. 아직 아무 시퀀스도 재생하지 않았으면 INDEX_NONE */
	int32 CurrentSequenceIndex = INDEX_NONE;

	/** 모든 시퀀스를 다 재생해 다음 레벨로 전환을 이미 요청했으면 true - 이후 입력은 무시해
	 *  OpenLevel()이 중복 호출되는 것을 막는다 */
	bool bHasFinished = false;

	virtual void NativeConstruct() override;

	/** 부모의 자동 전환(모든 입력에 반응)을 막기 위한 빈 오버라이드 - 실제 전환은 시퀀스를 모두
	 *  소진했을 때 HandleSequenceExhausted()가 직접 처리 */
	virtual void ScheduleSwitchToNextWidget() override;

	/** CutSceneDataTable의 모든 행을 SortedCutScenes로 복사한 뒤 Sequence_Index 오름차순으로 정렬 */
	void BuildSortedCutScenes();

	/** SortedCutScenes[0]가 있으면 재생 시작, 없으면 곧바로 HandleSequenceExhausted() 처리 */
	void PlayFirstSequence();

	/** Row의 Video/TextFont/FontSize/Text/ScriptBoxImage 중 비어있지 않은 필드만 화면에 반영하고
	 *  (비어있는 필드는 직전 값을 그대로 유지), bItalic은 항상 그 값 그대로(true/false) 적용한다.
	 *  Text에 포함된 리터럴 "\n"은 실제 개행 문자로 치환한 뒤 적용한다. 호출될 때마다(=시퀀스가
	 *  진행될 때마다) LogUI로 현재 진행 상황을 로그로 남긴다 */
	void ApplyCutSceneRow(const FCPCutSceneData& Row);

	/** OnAnyKeyPressed에 바인딩 - 다음 Sequence_Index로 진행시키거나(있으면 ApplyCutSceneRow),
	 *  더 이상 남은 시퀀스가 없으면 HandleSequenceExhausted() 호출 */
	UFUNCTION()
	void HandleAdvanceSequenceInput(FKey PressedKey);

	/** 모든 시퀀스를 다 재생했을 때 처리 - NextLevelName이 비어있지 않으면 bHasFinished를 세우고
	 *  UGameplayStatics::OpenLevel()로 다음 레벨을 불러온다 */
	void HandleSequenceExhausted();

	/** MediaPlayer::OnMediaOpened에 바인딩 - OpenSource() 요청이 실제로 성공했을 때(비동기) LogUI로
	 *  기록한다. OpenSource()의 반환값은 "요청이 접수됐는지"일 뿐이라 실제 성공 여부는 이 이벤트로
	 *  확인해야 함 */
	UFUNCTION()
	void HandleMediaOpened(FString OpenedUrl);

	/** MediaPlayer::OnMediaOpenFailed에 바인딩 - OpenSource() 요청이 비동기로 실패했을 때(코덱 미지원,
	 *  플러그인 미설정 등) LogUI로 경고를 남긴다 */
	UFUNCTION()
	void HandleMediaOpenFailed(FString FailedUrl);

	/** MediaPlayer::OnPlaybackResumed에 바인딩 - 실제로 재생이 시작됐을 때 LogUI로 기록한다.
	 *  OnMediaOpened까지는 성공했는데 이 로그가 안 찍히면 PlayOnOpen이 꺼져 있거나 자동재생이
	 *  차단된 상태일 수 있음 */
	UFUNCTION()
	void HandlePlaybackResumed();
};
