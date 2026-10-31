#include "Core/EscapeAreaVolume.h"
#include "Components/BoxComponent.h"

AEscapeAreaVolume::AEscapeAreaVolume()
{
	// 單位方塊（全長 100 cm）；實際大小用 Actor 的縮放（Level_Layout.csv 的 SX／SY／SZ ÷ 100）
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->InitBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	// 只 Overlap：查詢看得到，但不擋 Pawn、不擋 Visibility 的線
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Overlap);
	Box->SetGenerateOverlapEvents(true);
	SetRootComponent(Box);
	PrimaryActorTick.bCanEverTick = false;
}

bool AEscapeAreaVolume::ContainsPoint(const FVector& P) const
{
	if (!Box) { return false; }
	// 用盒子的世界變換（含縮放與旋轉）判斷：轉到盒子本地座標，再和縮放後的 extent 比
	const FVector Local = Box->GetComponentTransform().InverseTransformPositionNoScale(P);
	const FVector Extent = Box->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}
