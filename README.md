# 🧟 Dismemberment System for Unreal Engine

A robust, performance-optimized, and easy-to-use bone-based dismemberment plugin for Unreal Engine. Perfect for zombie, swarm, or creature-based games where limbs can be hidden and regrown dynamically.

## ✨ Features

- **🦴 Bone-Based Health**: Individually configure max health, health regeneration flags, and damage multipliers per bone.
- **🩸 Visual Dismemberment**: Dynamically hide skeletal mesh bones while triggering custom Niagara effects and sounds upon breaking a bone.
- **🧬 Regeneration System**: Bones that haven't been severed can passively heal over time. Severed limbs can fully regrow after a specified delay.
- **⚡ Performance Optimized**: The actor component operates without ticking until a regeneration or healing phase is required, at which point it activates and cleanly shuts itself down once the task is finished. Memory allocation is optimized to avoid garbage collection overloads.
- **🔄 Bone Redirection**: Redirect damage from a complex bone (like a fingertip) straight to a main bone (like the arm) using the built-in redirection map.
- **🔌 Blueprint Events**: Fully exposed event delegates (`OnBoneBroken`, `OnBoneRestored`, `OnBoneDamaged`) designed for seamless blueprint integrations, such as altering movement speeds when a leg is lost.

---

## 🚀 Installation

1. Close your Unreal Engine project.
2. Clone or copy the `DismembermentSystem` folder into the `Plugins/` folder of your project. If the folder doesn't exist, create it.
3. Open your project. When prompted, click **Yes** to rebuild the plugin modules.
4. Go to **Edit > Plugins**, search for **DismembermentSystem**, and verify it is enabled.

---

## 📖 Quick Start Guide

### 1. Adding Dismemberment to a Character
1. Open your Character blueprint.
2. Add an **AC_Dismemberment** component to the components list.
3. In the component properties, define your `BoneSettings` map, specifying the bone names (e.g., `hand_r`, `lowerarm_l`) and their attributes (Max Health, Damage Multiplier, Niagara System, etc.).
4. Optional: Configure `BoneRedirectMap` if you want hits on `finger_index_r` to redirect damage to `hand_r`.

### 2. Applying Damage
You can apply damage to specific limbs through C++ or Blueprint:
```cpp
// Apply damage to a specific bone
// The component will automatically handle health reduction, multipliers, and dismemberment
DismembermentComp->ApplyDismembermentDamage("head", 50.0f);
```

### 3. Immediate Dismemberment or Restoration
You can instantly chop off or restore a limb without dealing damage:
```cpp
// Instantly sever a limb and trigger visuals
DismembermentComp->DismemberBone("calf_l");

// Instantly restore a limb to full health and make it visible again
DismembermentComp->RestoreBone("calf_l");
```

### 4. Querying Status
Check if a bone is broken or get its current health:
```cpp
bool bIsBroken = DismembermentComp->IsBoneBroken("spine_03");
float CurrentHealth = DismembermentComp->GetBoneHealth("hand_l");
```

---

## 🏛️ Core Architecture

### `UAc_Dismemberment`
The main actor component that lives on your Character or Creature. It manages `BoneSettings`, handles damage logic, and fires delegates (`OnBoneBroken`, `OnBoneRestored`, `OnBoneDamaged`) whenever health changes or a bone is severed.

### `FBreakableBoneData`
The underlying struct that holds data for each breakable bone. It contains properties for:
- Max & Current Health
- Damage Multiplier
- Regeneration Flags
- Niagara System & Sound Effects
- Additional Bones to Hide (Useful for hiding child bones that might not be directly parented in the hierarchy)

---

## ⚙️ Technical Considerations

This component utilizes `HideBoneByName` and `UnHideBoneByName` on the owner's Skeletal Mesh to create the dismemberment effect. It does **not** physically drop the amputated limbs into the world as rigid bodies; it simply hides them for maximum optimization in high-density crowd scenarios.

> **Note on Modular Characters:** By default, it targets a single cached `USkeletalMeshComponent`. If you are using a Master Pose Component setup with multiple meshes (clothes, armors), additional logic will be required to synchronize the hidden bones across all attached sub-meshes.

---

## ⚖️ License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
