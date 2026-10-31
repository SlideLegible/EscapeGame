// 步驟 9 灰盒關卡：玩家的人類身體與可變身物品的 Pawn（移動與鏡頭是步驟 11）。
// 現在是空殼，只讓 Step9_Level.spec.cpp 編得過。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EscapeFormPawn.generated.h"

class UStaticMeshComponent;

UCLASS()
class AEscapeFormPawn : public APawn
{
	GENERATED_BODY()

public:
	AEscapeFormPawn();

	/** F00～F12 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FString FormId;

	/** RootComponent */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
