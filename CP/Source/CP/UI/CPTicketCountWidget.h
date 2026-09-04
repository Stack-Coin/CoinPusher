// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPTicketCountWidget.generated.h"

class UTextBlock;

/**
 *  "티켓 X개" 형태로 보유 티켓 개수를 표시하는 UI 클래스.
 *  티켓 개수 변경 델리게이트(ACPGameMode::OnTeamTicketCountChanged)를 UpdateTicketCount에
 *  바인딩해두면, Broadcast될 때마다 자동으로 텍스트가 갱신된다.
 */
UCLASS(abstract)
class CP_API UCPTicketCountWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	/** 개수를 표시할 TextBlock. 없어도 동작은 하지만(텍스트 갱신만 생략) BP에서 배치를 권장 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TicketCountText;

	/** TicketCountText에 적용할 표시 형식. {0} 자리에 개수가 들어간다.
	 *  기본값은 자리표시자일 뿐 - 실제 문구(예: "티켓 {0}개")는 이 클래스를 상속하는
	 *  Widget Blueprint의 Class Defaults에서 지정한다 (소스 코드에 한글 리터럴을 직접
	 *  넣으면 컴파일러 소스 인코딩에 따라 깨질 수 있어 피한다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ticket")
	FText DisplayFormat = FText::FromString(TEXT("Ticket x{0}"));

public:

	/** 티켓 개수 변경 델리게이트에 바인딩해서 쓰는 진입점 */
	UFUNCTION(BlueprintCallable, Category="Ticket")
	void UpdateTicketCount(int32 Count);
};
