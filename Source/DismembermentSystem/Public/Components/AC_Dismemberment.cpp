#include "AC_Dismemberment.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

DEFINE_LOG_CATEGORY(LogDismemberment);

UAc_Dismemberment::UAc_Dismemberment() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAc_Dismemberment::BeginPlay() {
  Super::BeginPlay();

  AActor *Owner = GetOwner();
  if (!Owner)
    return;

  if (ACharacter *Character = Cast<ACharacter>(Owner)) {
    CachedMesh = Character->GetMesh();
  } else {
    CachedMesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
  }

  for (auto &Elem : BoneSettings) {
    Elem.Value.CurrentHealth = Elem.Value.MaxHealth;
  }
}

void UAc_Dismemberment::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  UWorld *World = GetWorld();
  if (!World || !bEnableSystemRegen)
    return;

  double CurrentTime = World->GetTimeSeconds();
  bool bNeedsUpdate = false;

  // Use cached array to avoid repeated allocations
  BonesToRestoreCache.Reset();

  for (const auto &Pair : BrokenBonesMap) {
    FName BoneName = Pair.Key;
    double BreakTime = Pair.Value;

    if (FBreakableBoneData *Data = BoneSettings.Find(BoneName)) {
      if (Data->bCanRegenerate &&
          (CurrentTime - BreakTime >= LimbRegrowDelay)) {
        BonesToRestoreCache.Add(BoneName);
      }
    }
  }

  for (const FName &BoneToRestore : BonesToRestoreCache) {
    RestoreBone(BoneToRestore);
  }

  if (BrokenBonesMap.Num() > 0) {
    bNeedsUpdate = true;
  }

  if (CurrentTime - LastDamageTime >= PassiveHealDelay) {
    for (auto &Elem : BoneSettings) {
      if (BrokenBonesMap.Contains(Elem.Key))
        continue;

      if (Elem.Value.CurrentHealth < Elem.Value.MaxHealth) {
        Elem.Value.CurrentHealth = FMath::Min(
            Elem.Value.MaxHealth,
            Elem.Value.CurrentHealth + (HealAmountPerSecond * DeltaTime));
        bNeedsUpdate = true;
      }
    }
  }

  if (!bNeedsUpdate && BrokenBonesMap.Num() == 0) {
    SetComponentTickEnabled(false);
  }
}

FName UAc_Dismemberment::ResolveBoneName(const FName &HitBoneName) const {
  if (const FName *MappedBone = BoneRedirectMap.Find(HitBoneName)) {
    return *MappedBone;
  }
  return HitBoneName;
}

void UAc_Dismemberment::ApplyDismembermentDamage(FName HitBoneName,
                                                 float DamageAmount) {
  if (HitBoneName.IsNone())
    return;

  const FName TargetBone = ResolveBoneName(HitBoneName);

  if (BrokenBonesMap.Contains(TargetBone))
    return;

  FBreakableBoneData *BoneData = BoneSettings.Find(TargetBone);
  if (!BoneData)
    return;

  if (UWorld *World = GetWorld()) {
    LastDamageTime = World->GetTimeSeconds();
  }
  SetComponentTickEnabled(true);

  // Apply damage multiplier
  const float ActualDamage = DamageAmount * BoneData->DamageMultiplier;
  const float OldHealth = BoneData->CurrentHealth;
  BoneData->CurrentHealth =
      FMath::Max(MinHealth, BoneData->CurrentHealth - ActualDamage);

  // Broadcast damage event
  OnBoneDamaged.Broadcast(TargetBone, BoneData->CurrentHealth, ActualDamage);

  if (BoneData->CurrentHealth <= MinHealth) {
    DismemberBone(TargetBone);
  }
}

void UAc_Dismemberment::DismemberBone(FName BoneName) {
  if (BrokenBonesMap.Contains(BoneName))
    return;

  FBreakableBoneData *BoneData = BoneSettings.Find(BoneName);
  if (!BoneData)
    return;

  // Kırılma anını kaydet (Şimdiki Zaman)
  if (UWorld *World = GetWorld()) {
    BrokenBonesMap.Add(BoneName, World->GetTimeSeconds());
  }

  // Regen sistemi açıksa Tick'i çalışır durumda tutmamız lazım ki süreyi saysın
  if (bEnableSystemRegen && BoneData->bCanRegenerate) {
    SetComponentTickEnabled(true);
  }

  HandleVisualEffects(BoneName, *BoneData);
  HideBoneHierarchy(BoneName, BoneData->AdditionalBonesToHide, true);

  OnBoneBroken.Broadcast(BoneName);
}

void UAc_Dismemberment::RestoreBone(FName BoneName) {
  FBreakableBoneData *BoneData = BoneSettings.Find(BoneName);
  if (!BoneData)
    return;

  if (BrokenBonesMap.Remove(BoneName) > 0) {
    BoneData->CurrentHealth = BoneData->MaxHealth;
    HideBoneHierarchy(BoneName, BoneData->AdditionalBonesToHide, false);
    OnBoneRestored.Broadcast(BoneName);
  }
}

void UAc_Dismemberment::HandleVisualEffects(
    const FName &BoneName, const FBreakableBoneData &Data) const {
  if (!CachedMesh)
    return;

  // Validate bone exists before accessing
  if (CachedMesh->GetBoneIndex(BoneName) == INDEX_NONE)
    return;

  const FVector Location = CachedMesh->GetBoneLocation(BoneName);
  if (Data.SoundToPlay)
    UGameplayStatics::PlaySoundAtLocation(this, Data.SoundToPlay, Location);
  if (Data.NiagaraSystem) {
    const FRotator Rotation = CachedMesh->GetBoneQuaternion(BoneName).Rotator();
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        GetWorld(), Data.NiagaraSystem, Location, Rotation);
  }
}

void UAc_Dismemberment::HideBoneHierarchy(const FName &BoneName,
                                          const TArray<FName> &ChildBones,
                                          bool bHide) const {
  if (!CachedMesh)
    return;

  // Lambda helper to reduce code duplication
  auto ToggleBone = [this, bHide](const FName &Bone) {
    if (CachedMesh->GetBoneIndex(Bone) != INDEX_NONE) {
      if (bHide)
        CachedMesh->HideBoneByName(Bone, PBO_None);
      else
        CachedMesh->UnHideBoneByName(Bone);
    }
  };

  ToggleBone(BoneName);
  for (const FName &Child : ChildBones) {
    ToggleBone(Child);
  }
}

bool UAc_Dismemberment::IsBoneBroken(FName BoneName) const {
  return BrokenBonesMap.Contains(BoneName);
}

float UAc_Dismemberment::GetBoneHealth(FName BoneName) const {
  if (const FBreakableBoneData *BoneData = BoneSettings.Find(BoneName)) {
    return BoneData->CurrentHealth;
  }
  return 0.0f;
}

#if WITH_EDITOR
void UAc_Dismemberment::DrawDebugBoneHealth() const {
  if (!CachedMesh || !GetWorld())
    return;

  for (const auto &Pair : BoneSettings) {
    const FName &BoneName = Pair.Key;
    const FBreakableBoneData &Data = Pair.Value;

    if (CachedMesh->GetBoneIndex(BoneName) == INDEX_NONE)
      continue;

    const FVector BoneLocation = CachedMesh->GetBoneLocation(BoneName);
    const float HealthPercent = Data.CurrentHealth / Data.MaxHealth;
    const FColor HealthColor =
        FColor::MakeRedToGreenColorFromScalar(HealthPercent);

    DrawDebugString(GetWorld(), BoneLocation + FVector(0, 0, 20),
                    FString::Printf(TEXT("%s: %.0f/%.0f"), *BoneName.ToString(),
                                    Data.CurrentHealth, Data.MaxHealth),
                    nullptr, HealthColor, 0.0f, true);
  }
}
#endif
