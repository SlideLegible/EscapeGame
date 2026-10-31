// 步驟 9 灰盒關卡：區域盒子（Classroom、Corridor… 8 個區域各一個）。
// 現在是空殼，只讓 Step9_Level.spec.cpp 編得過；ContainsPoint 的實作在 9b。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeAreaVolume.generated.h"

class UBoxComponent;

UCLASS()
class AEscapeAreaVolume : public AActor
{
	GENERATED_BODY()

public:
	AEscapeAreaVolume();

	/** 區域 ID：Classroom、Corridor、Office、Restroom、Storage、GuardRoom、L01、L02 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FName AreaId;

	/** 區域範圍（RootComponent）；碰撞只 Overlap，不擋 Pawn */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	TObjectPtr<UBoxComponent> Box;

	/** 世界座標的點在不在這個區域盒子裡 */
	bool ContainsPoint(const FVector& P) const;
};
