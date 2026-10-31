// 步驟 5 大門密碼：組合層。UE 端只跟它說話。純邏輯，只依賴 Core。
// 變身（6）、場景狀態（7）、成就計數（8）還沒做；玩家形態先固定 F00。
#pragma once

#include "CoreMinimal.h"
#include "Logic/InteractionResolver.h"
#include "Logic/TeamState.h"

enum class EInteractKind : uint8
{
	Rejected,       // 玩家不存在或已結束：什麼都不做
	Blocked,        // 被擋下（步驟 6：物品被別人占用）
	NeedsPassword,  // 要輸入密碼，狀態沒改
	Resolved,       // 互動成立，狀態已更新
};

struct FInteractOutcome
{
	EInteractKind Kind = EInteractKind::Rejected;
	FInteractionResult Result;
	TArray<FString> Messages;  // 依序顯示的文字
	TArray<FTeamEvent> Events;
	bool bHpChanceHit = false;
};

class FEscapeGameLogic
{
public:
	/** 載入五份 CSV 文字。機率扣血的列（X017）文字必須剛好有兩段「…」，否則回傳 false 並寫出互動 ID。 */
	static bool Load(const FString& InteractionsCsv, const FString& FormsCsv, const FString& SceneObjectsCsv,
		const FString& ResponsesCsv, const FString& LocationsCsv, int32 Seed, FEscapeGameLogic& Out, FString& OutError);

	void AddPlayer(const FString& PlayerId, const FVector& Spawn);

	FInteractOutcome Interact(const FString& PlayerId, const FString& TargetId);
	/** 在大門輸入四位數密碼。玩家不是人類形態（F00）→ Rejected */
	FInteractOutcome SubmitPassword(const FString& PlayerId, const FString& Code);

	TArray<FTeamEvent> Tick(float Dt);

	const FTeamState& GetTeam() const { return Team; }
	const FInteractionResolver& GetResolver() const { return Resolver; }

private:
	struct FPlayerInfo
	{
		FString PlayerId;
		FString FormId = TEXT("F00");
		FVector Location = FVector::ZeroVector;
	};

	const FPlayerInfo* FindPlayer(const FString& PlayerId) const;
	FInteractOutcome Apply(const FString& PlayerId, const FInteractionResult& Result);

	FInteractionResolver Resolver;
	FTeamState Team;
	FString LocationsCsv;
	FString PasswordTargetId;  // 密碼鎖的對象（S01），從 Resolver 資料找
	TArray<FPlayerInfo> Players;
};
