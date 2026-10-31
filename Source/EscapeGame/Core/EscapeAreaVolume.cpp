#include "Core/EscapeAreaVolume.h"
#include "Components/BoxComponent.h"

AEscapeAreaVolume::AEscapeAreaVolume()
{
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	SetRootComponent(Box);
}

bool AEscapeAreaVolume::ContainsPoint(const FVector& P) const
{
	return false;  // 空殼：9b 實作
}
