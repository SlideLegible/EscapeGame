// 步驟 9 灰盒關卡：關卡裡的場景物件與可變身物品（灰盒方塊）。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeObjectActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class AEscapeObjectActor : public AActor
{
	GENERATED_BODY()

public:
	AEscapeObjectActor();

	/** F01～F12、S01～S20 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FString ObjectId;

	/** S19／S05／S09 為真：擋路的門 */
	UPROPERTY(EditAnywhere, Category = "Escape")
	bool bIsDoor = false;

	/** RootComponent；預設 /Engine/BasicShapes/Cube，BlockAll */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 開＝關掉碰撞並隱藏；關＝恢復 */
	void SetDoorOpen(bool bOpen);
	bool IsDoorOpen() const;

	/** 步驟 10 場景同步：可變身物品（F01～F12）還沒出現＝隱藏且沒有碰撞；出現＝顯示並恢復碰撞（BlockAll） */
	void SetItemPresent(bool bPresent);
	bool IsItemPresent() const;

private:
	bool bDoorOpen = false;
	bool bItemPresent = true;
};
