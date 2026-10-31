#include "Forms/EscapeFormPawn.h"
#include "Components/StaticMeshComponent.h"

AEscapeFormPawn::AEscapeFormPawn()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}
