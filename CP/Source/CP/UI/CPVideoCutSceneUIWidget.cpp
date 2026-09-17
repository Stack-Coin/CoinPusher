// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CPVideoCutSceneUIWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "FileMediaSource.h"
#include "Kismet/GameplayStatics.h"
#include "Log/CPLogCategories.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"

void UCPVideoCutSceneUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (VideoImage && MediaTexture)
	{
		VideoImage->SetBrushResourceObject(MediaTexture);
	}
	else if (VideoImage && !MediaTexture)
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPVideoCutSceneUIWidget] MediaTexture가 지정되지 않았습니다. WBP에서 VideoImage의 Brush를 이미 Media Texture로 직접 지정해뒀는지 확인하세요 - 안 그러면 화면에 아무 것도 안 보입니다."));
	}

	if (MediaPlayer)
	{
		MediaPlayer->OnMediaOpened.AddDynamic(this, &UCPVideoCutSceneUIWidget::HandleMediaOpened);
		MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &UCPVideoCutSceneUIWidget::HandleMediaOpenFailed);
		MediaPlayer->OnPlaybackResumed.AddDynamic(this, &UCPVideoCutSceneUIWidget::HandlePlaybackResumed);
	}
	else
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPVideoCutSceneUIWidget] MediaPlayer가 지정되지 않았습니다. WBP Class Defaults에서 Media Player 에셋을 지정해야 영상이 재생됩니다."));
	}

	BuildSortedCutScenes();
	PlayFirstSequence();

	OnAnyKeyPressed.AddDynamic(this, &UCPVideoCutSceneUIWidget::HandleAdvanceSequenceInput);
}

void UCPVideoCutSceneUIWidget::ScheduleSwitchToNextWidget()
{
	// 아무 것도 하지 않음 - 부모가 매 입력마다 자동으로 부르지만, 이 위젯은 모든 시퀀스를 소진했을
	// 때만 OpenLevel로 전환해야 하므로 HandleSequenceExhausted()가 직접 처리한다
}

void UCPVideoCutSceneUIWidget::BuildSortedCutScenes()
{
	SortedCutScenes.Reset();

	if (!CutSceneDataTable)
	{
		UE_LOG(LogUI, Warning, TEXT("[UCPVideoCutSceneUIWidget] CutSceneDataTable이 지정되지 않았습니다. WBP Class Defaults에서 CutSceneDataTable을 지정하세요."));
		return;
	}

	CutSceneDataTable->ForeachRow<FCPCutSceneData>(TEXT("UCPVideoCutSceneUIWidget"),
		[this](const FName& RowName, const FCPCutSceneData& Row)
		{
			SortedCutScenes.Add(Row);
		});

	SortedCutScenes.Sort([](const FCPCutSceneData& A, const FCPCutSceneData& B)
	{
		return A.Sequence_Index < B.Sequence_Index;
	});

	UE_LOG(LogUI, Log, TEXT("[UCPVideoCutSceneUIWidget] CutSceneDataTable(%s)에서 %d개의 시퀀스를 불러왔습니다."),
		*GetNameSafe(CutSceneDataTable), SortedCutScenes.Num());
}

void UCPVideoCutSceneUIWidget::PlayFirstSequence()
{
	CurrentSequenceIndex = 0;

	if (SortedCutScenes.IsValidIndex(CurrentSequenceIndex))
	{
		ApplyCutSceneRow(SortedCutScenes[CurrentSequenceIndex]);
	}
	else
	{
		HandleSequenceExhausted();
	}
}

void UCPVideoCutSceneUIWidget::ApplyCutSceneRow(const FCPCutSceneData& Row)
{
	UE_LOG(LogUI, Log, TEXT("[UCPVideoCutSceneUIWidget] 시퀀스 진행: %d/%d (CutSceneID=%s, Sequence_Index=%d)"),
		CurrentSequenceIndex + 1, SortedCutScenes.Num(), *Row.CutSceneID.ToString(), Row.Sequence_Index);

	if (Row.Video)
	{
		if (MediaPlayer)
		{
			const bool bOpened = MediaPlayer->OpenSource(Row.Video);
			UE_LOG(LogUI, Log, TEXT("[UCPVideoCutSceneUIWidget] MediaPlayer->OpenSource(%s) 결과: %s"),
				*GetNameSafe(Row.Video), bOpened ? TEXT("성공") : TEXT("실패"));
		}
		else
		{
			UE_LOG(LogUI, Warning, TEXT("[UCPVideoCutSceneUIWidget] Row.Video(%s)가 지정돼 있지만 MediaPlayer가 없어 재생할 수 없습니다."),
				*GetNameSafe(Row.Video));
		}
	}

	if (Row.TextFont.HasValidFont() && ScriptText)
	{
		ScriptText->SetFont(Row.TextFont);
	}

	if (Row.FontSize > 0.0f && ScriptText)
	{
		ScriptText->SetFontSize(Row.FontSize);
	}

	if (ScriptText)
	{
		const float DesiredSkewAmount = Row.bItalic ? ItalicSkewAmount : 0.0f;
		FSlateFontInfo UpdatedFont = ScriptText->GetFont();
		if (UpdatedFont.SkewAmount != DesiredSkewAmount)
		{
			UpdatedFont.SkewAmount = DesiredSkewAmount;
			ScriptText->SetFont(UpdatedFont);
		}
	}

	if (!Row.Text.IsEmpty() && ScriptText)
	{
		// DataTable 에디터에서 셀에 입력한 "\n"은 실제 줄바꿈 문자가 아니라 백슬래시+n 두 글자
		// 그대로 문자열에 들어가므로, 표시 직전에 실제 개행 문자로 치환해줘야 줄바꿈이 적용된다
		FString ProcessedText = Row.Text.ToString().Replace(TEXT("\\n"), TEXT("\n"));
		ScriptText->SetText(FText::FromString(MoveTemp(ProcessedText)));
	}

	if (Row.ScriptBoxImage && ScriptBackgroundImage)
	{
		ScriptBackgroundImage->SetBrushFromTexture(Row.ScriptBoxImage);
	}
}

void UCPVideoCutSceneUIWidget::HandleAdvanceSequenceInput(FKey PressedKey)
{
	if (bHasFinished)
	{
		return;
	}

	++CurrentSequenceIndex;

	if (SortedCutScenes.IsValidIndex(CurrentSequenceIndex))
	{
		ApplyCutSceneRow(SortedCutScenes[CurrentSequenceIndex]);
	}
	else
	{
		HandleSequenceExhausted();
	}
}

void UCPVideoCutSceneUIWidget::HandleSequenceExhausted()
{
	if (bHasFinished || NextLevelName.IsNone())
	{
		return;
	}

	bHasFinished = true;
	UGameplayStatics::OpenLevel(this, NextLevelName);
}

void UCPVideoCutSceneUIWidget::HandleMediaOpened(FString OpenedUrl)
{
	UE_LOG(LogUI, Log, TEXT("[UCPVideoCutSceneUIWidget] MediaPlayer OnMediaOpened: %s"), *OpenedUrl);
}

void UCPVideoCutSceneUIWidget::HandleMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogUI, Warning, TEXT("[UCPVideoCutSceneUIWidget] MediaPlayer OnMediaOpenFailed: %s - 영상 파일 경로/코덱과 Electra Player 플러그인 활성화 여부를 확인하세요."), *FailedUrl);
}

void UCPVideoCutSceneUIWidget::HandlePlaybackResumed()
{
	UE_LOG(LogUI, Log, TEXT("[UCPVideoCutSceneUIWidget] MediaPlayer OnPlaybackResumed - 실제로 재생이 시작됐습니다."));
}
