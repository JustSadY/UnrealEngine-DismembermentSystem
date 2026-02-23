#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "AC_Dismemberment.generated.h"

class UNiagaraSystem;
class USoundBase;

DECLARE_LOG_CATEGORY_EXTERN(LogDismemberment, Log, All);

USTRUCT(BlueprintType)
struct FBreakableBoneData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
  float MaxHealth = 100.0f;

  float CurrentHealth = 100.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
  float DamageMultiplier = 1.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
  bool bCanRegenerate = false;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
  UNiagaraSystem *NiagaraSystem = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX")
  USoundBase *SoundToPlay = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visibility")
  TArray<FName> AdditionalBonesToHide;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoneBroken, FName, BoneName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoneRestored, FName, BoneName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBoneDamaged, FName, BoneName,
                                               float, RemainingHealth, float,
                                               DamageAmount);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DISMEMBERMENTSYSTEM_API UAc_Dismemberment : public UActorComponent {
  GENERATED_BODY()

public:
  UAc_Dismemberment();

  UPROPERTY(BlueprintAssignable, Category = "Dismemberment")
  FOnBoneBroken OnBoneBroken;

  UPROPERTY(BlueprintAssignable, Category = "Dismemberment")
  FOnBoneRestored OnBoneRestored;

  UPROPERTY(BlueprintAssignable, Category = "Dismemberment")
  FOnBoneDamaged OnBoneDamaged;

protected:
  virtual void BeginPlay() override;
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
  TMap<FName, FBreakableBoneData> BoneSettings;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
  TMap<FName, FName> BoneRedirectMap;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
  bool bEnableSystemRegen = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
  float LimbRegrowDelay = 5.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
  float PassiveHealDelay = 3.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regeneration")
  float HealAmountPerSecond = 10.0f;
  // ----------------------------

  UPROPERTY(VisibleInstanceOnly, Category = "Debug")
  TMap<FName, double> BrokenBonesMap;

  UPROPERTY()
  class USkeletalMeshComponent *CachedMesh;

  // Cache for bone restoration to avoid repeated allocations
  TArray<FName> BonesToRestoreCache;

private:
  FName ResolveBoneName(const FName &HitBoneName) const;
  void HandleVisualEffects(const FName &BoneName,
                           const FBreakableBoneData &Data) const;
  void HideBoneHierarchy(const FName &BoneName, const TArray<FName> &ChildBones,
                         bool bHide) const;

  float LastDamageTime = 0.0f;

public:
  UFUNCTION(BlueprintCallable, Category = "Dismemberment")
  void ApplyDismembermentDamage(FName HitBoneName, float DamageAmount);

  UFUNCTION(BlueprintCallable, Category = "Dismemberment")
  void DismemberBone(FName BoneName);

  UFUNCTION(BlueprintCallable, Category = "Dismemberment")
  void RestoreBone(FName BoneName);

  UFUNCTION(BlueprintPure, Category = "Dismemberment")
  bool IsBoneBroken(FName BoneName) const;

  UFUNCTION(BlueprintPure, Category = "Dismemberment")
  float GetBoneHealth(FName BoneName) const;

#if WITH_EDITOR
  void DrawDebugBoneHealth() const;
#endif

private:
  static constexpr float MinHealth = 0.0f;
};
