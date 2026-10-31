#include "Forms/EscapeObjectActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AEscapeObjectActor::AEscapeObjectActor()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// 灰盒：預設是引擎內建的 100 cm 方塊；實際大小用 Actor 縮放（Level_Layout.csv 的 SX／SY／SZ ÷ 100）
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) { Mesh->SetStaticMesh(CubeMesh.Object); }
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	PrimaryActorTick.bCanEverTick = false;
}

void AEscapeObjectActor::SetDoorOpen(bool bOpen)
{
	bDoorOpen = bOpen;
	// 開＝隱藏並關掉碰撞；關＝顯示並擋 Pawn
	SetActorHiddenInGame(bOpen);
	Mesh->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
}

bool AEscapeObjectActor::IsDoorOpen() const
{
	return bDoorOpen;
}
