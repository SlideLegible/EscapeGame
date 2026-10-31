// 步驟 2 互動判定（紅階段：尚未實作）
#include "Logic/InteractionResolver.h"

bool FInteractionResolver::Load(const FString&, const FString&, const FString&, const FString&,
	FInteractionResolver&, FString& OutError)
{
	OutError = TEXT("尚未實作");
	return false;
}

FInteractionResult FInteractionResolver::Resolve(const FInteractionQuery&) const
{
	return FInteractionResult();
}

FString FInteractionResolver::GetDisplayName(const FString& Id) const
{
	return Id;
}
