#include "Forms/EscapeFormPawn.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AEscapeFormPawn::AEscapeFormPawn()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// 灰盒：預設是引擎內建的方塊，可以在編輯器或 Blueprint 換成別的網格
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) { Mesh->SetStaticMesh(CubeMesh.Object); }
	Mesh->SetCollisionProfileName(TEXT("Pawn"));
	PrimaryActorTick.bCanEverTick = false;
}
