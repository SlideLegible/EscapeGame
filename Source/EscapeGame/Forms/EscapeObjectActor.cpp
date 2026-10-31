#include "Forms/EscapeObjectActor.h"
#include "Components/StaticMeshComponent.h"

AEscapeObjectActor::AEscapeObjectActor()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void AEscapeObjectActor::SetDoorOpen(bool bOpen)
{
	// 空殼：9b 實作
}

bool AEscapeObjectActor::IsDoorOpen() const
{
	return bDoorOpen;
}
