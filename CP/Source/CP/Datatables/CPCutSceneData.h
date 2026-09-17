// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Fonts/SlateFontInfo.h"
#include "CPCutSceneData.generated.h"

class UFileMediaSource;
class UTexture2D;

/** 컷신 데이터 한 행. UI/CPVideoCutSceneUIWidget.h의 UCPVideoCutSceneUIWidget::CutSceneDataTable의
 *  Row Struct로 쓰인다. 한 행 = 컷신 진행 중 한 시퀀스(영상/텍스트 조합) */
USTRUCT(BlueprintType)
struct FCPCutSceneData : public FTableRowBase
{
	GENERATED_BODY()

	/** 식별용 컷신 ID. DataTable의 Row Name과 동일한 값으로 등록해두면 가독성/참조에 도움이 된다
	 *  (실제 재생 순서는 이 필드가 아니라 Sequence_Index 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	FName CutSceneID;

	/** 이 시퀀스에서 재생할 영상. 비어있으면(nullptr) 직전 시퀀스에서 재생 중이던 영상을 그대로
	 *  이어서 보여준다(UCPVideoCutSceneUIWidget이 새 값이 나올 때까지 이전 값 유지) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	TObjectPtr<UFileMediaSource> Video = nullptr;

	/** 이 시퀀스의 Text에 적용할 폰트. 유효한 폰트가 지정되지 않았으면(FSlateFontInfo::HasValidFont()
	 *  false) 직전 시퀀스에서 쓰던 폰트를 그대로 유지한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	FSlateFontInfo TextFont;

	/** 이 시퀀스의 Text에 적용할 폰트 크기. TextFont(폰트 자체)는 그대로 두고 크기만 바꾸고 싶을 때
	 *  사용 - 0 이하면 "지정 안 함"으로 취급해 직전 시퀀스에서 쓰던 크기를 그대로 유지한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	float FontSize = 0.0f;

	/** 이 시퀀스의 Text를 이탤릭(기울임)으로 표시할지 여부. bool이라 "지정 안 함" 상태가 없으므로
	 *  Video/TextFont/FontSize/Text/ScriptBoxImage와 달리 이전 값을 유지하지 않고 매 시퀀스마다
	 *  이 값 그대로(true/false) 적용된다 - 이전 시퀀스에서 이탤릭이었어도 다음 시퀀스에서 false면
	 *  즉시 해제됨 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	bool bItalic = false;

	/** 이 시퀀스에서 보여줄 대사/설명 텍스트. 비어있으면 직전 시퀀스의 텍스트를 그대로 유지한다.
	 *  줄바꿈하고 싶은 위치에 "\n"을 입력하면 UCPVideoCutSceneUIWidget이 실제 개행 문자로 바꿔서
	 *  적용해준다(DataTable 에디터에서 Enter로 실제 개행을 입력하기 번거로워 이 방식을 지원) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	FText Text;

	/** 대사/설명 텍스트 뒤에 깔리는 스크립트창 배경 이미지. 비어있으면(nullptr) 직전 시퀀스에서
	 *  쓰던 스크립트창 이미지를 그대로 유지한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	TObjectPtr<UTexture2D> ScriptBoxImage = nullptr;

	/** 컷신 재생 순서. UCPVideoCutSceneUIWidget이 DataTable의 모든 행을 이 값 오름차순으로 정렬해
	 *  순서대로 재생한다(Row Name이나 DataTable 등록 순서와 무관) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CutScene Data")
	int32 Sequence_Index = 0;
};
