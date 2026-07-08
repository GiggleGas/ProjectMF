# 物品引用下拉选择（FMFItemKey）方案

> 2026-07-07 ｜ 目标：DT_Item 被其他配置（掉落表 / 配方 / AIConfig 等）引用时，编辑器里**下拉点选**而非手输数字 ItemID。
> 状态：方向已确认（2026-07-07 用户拍板走精简版），下方 §6 为详细实施方案。参考 F1 项目 `F1DataDefinition.h`，取核心砍重型。

---

## 1. F1 框架拆解（参考对象）

F1 的 `F1DATAKEY` = 强类型 key struct（`FF1DataKey_Int/FName`）+ `TF1DataKeyTrait`（Resolve + 编辑器 Display 映射）+ 配套 PropertyCustomization（下拉控件）。为通用性它还背了：Angelscript 绑定、Data Registry 多源、`GenerateIntKey`（RowName 非数字时自定义 int 键）、rename 重定向、`MarkSearchableName` 引用追踪。

**ProjectMF 用不到这些**：无 Angelscript、无 Data Registry、RowName 本身就是数字=ItemID。所以取其三件套核心即可，其余全砍。

## 2. 推荐方案：精简版 `FMFItemKey`

### 2.1 Runtime：一个包装 struct（Inventory 模块）

```cpp
// MFItemKey.h
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFItemKey
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    int32 ItemID = 0;

    const FMFItemDef* Resolve() const { return UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), ItemID); }
    bool IsValid() const { return ItemID > 0; }
    bool operator==(const FMFItemKey& O) const { return ItemID == O.ItemID; }
    friend uint32 GetTypeHash(const FMFItemKey& K) { return ::GetTypeHash(K.ItemID); }
};
```

数据上它 == 一个 int32，序列化/存档零负担；语义上它是"物品引用"，带 `Resolve()`。

### 2.2 Editor：一个 PropertyTypeCustomization（下拉控件）

新建**编辑器模块** `ProjectMFEditor`（见 §3），里面：

```cpp
// FMFItemKeyCustomization : IPropertyTypeCustomization
//   CustomizeHeader：把 FMFItemKey 画成一个 SSearchableComboBox
//     选项 = 遍历 UMFItemSettings::GetItemTable() 每行 → "DisplayName (#ItemID)"
//     选中 → 写回内部 ItemID 属性
//     当前值 → 显示对应行的 DisplayName（查不到显示 "#ID (缺失)"）
// StartupModule: RegisterCustomPropertyTypeLayout("MFItemKey", ...)
```

物品多时 `SSearchableComboBox` 可搜索；这正是 F1 `GetDisplayNames`/`LUT` 喂给下拉的那套，只是数据源直接读 DT_Item、不经 Trait 缓存层。

### 2.3 引用处：int32 → FMFItemKey

- `FMFLootEntry.ItemID`（int32）→ `FMFItemKey Item`
- `FMFRecipeDef.Inputs`/`OutputItemID`（打造，A1.8）→ `FMFItemKey`
- 运行时取 `Entry.Item.ItemID` 或 `Entry.Item.Resolve()`（改动小，语义更清晰）

已落地的裸 int32（如 `MFSpawnLoot` 参数、背包 slot 内部）**不必动**——那些是运行时数据/调试入口，不是编辑器配置面。只在"编辑器要配物品引用"的地方用 FMFItemKey。

## 3. 成本：需要新建编辑器模块

ProjectMF 目前只有一个 runtime 模块（`ProjectMF`），没有编辑器模块。PropertyTypeCustomization 必须放在编辑器模块。所以前置成本 = **新建 `ProjectMFEditor` 模块**：
- `ProjectMFEditor.Build.cs`（依赖 UnrealEd / PropertyEditor / Slate）
- `ProjectMFEditor.cpp/.h`（StartupModule 注册 customization）
- uproject Modules 增一项（Type=Editor）
- `FMFItemKeyCustomization.cpp/.h`

这是一次性基础设施；建好后，将来任何"下拉选资产/枚举/自定义 struct"的编辑器体验都能往这里加。

## 4. 更轻的备选（不建编辑器模块）

`meta=(GetOptions="StaticFunc")`：给属性提供下拉候选，纯 runtime 反射、无需 customization。**但**：
- GetOptions 适配 FName 属性，返回 `TArray<FName>`；对 int32 ItemID 不直接匹配。
- 若把引用改回 FName RowName（数字字符串），可用；但下拉显示的是 "1001" 而非 "治疗果"，可读性差，且相当于回退 int32 化。

结论：GetOptions 省一个模块，但体验明显差（无物品名、只有数字）。要"像 F1 那样点选带名字的下拉"，还得走 §2 的 customization。

## 5. 建议

| | 精简版 FMFItemKey（§2） | GetOptions（§4） |
|---|---|---|
| 下拉体验 | ✅ 带物品名、可搜索 | ⚠️ 只有数字 RowName |
| 前置成本 | 新建 editor 模块（一次性） | 无 |
| 数据/存档 | int32 等价，零负担 | 需 ItemID 回 FName |
| 扩展性 | ✅ 以后所有下拉都能加这 | 有限 |

**推荐精简版 FMFItemKey**：它就是 F1 框架的正确精简——留下"包装 struct + Resolve + 下拉 customization"，砍掉 Angelscript/DataRegistry/引用追踪/rename 那些 ProjectMF 不需要的重量。一次建好 editor 模块，后续掉落表、配方、装备等所有引用物品的地方都能下拉点选。

**排期考量**：这是"配置体验"优化，不是月底测试版闭环的必需项（当前掉落表手填数字 ItemID 能跑）。可以：①现在做（W2 打造前做完，配方也能享受下拉）；②或先记为技术债，测试版闭环后再补。倾向 ①——打造系统 A1.8 马上要配大量物品引用，下拉能省很多手输和填错。

---

## 6. 详细实施方案

### 6.1 新建编辑器模块 `ProjectMFEditor`

现状：`ProjectMFEditor.Target.cs` 已存在（Editor target），但只 `ExtraModuleNames.Add("ProjectMF")`，没有独立编辑器模块。新增：

**① `ProjectMF.uproject`** — Modules 数组加一项（放在 ProjectMF 之后）：
```json
{ "Name": "ProjectMFEditor", "Type": "Editor", "LoadingPhase": "Default" }
```

**② `Source/ProjectMFEditor/ProjectMFEditor.Build.cs`**：
```csharp
public class ProjectMFEditor : ModuleRules {
  public ProjectMFEditor(ReadOnlyTargetRules Target) : base(Target) {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
    PublicDependencyModuleNames.AddRange(new[]{ "Core", "CoreUObject", "Engine", "ProjectMF" });
    PrivateDependencyModuleNames.AddRange(new[]{ "UnrealEd", "PropertyEditor", "Slate", "SlateCore" });
  }
}
```

**③ `Source/ProjectMFEditor/Public/ProjectMFEditor.h` + `Private/ProjectMFEditor.cpp`** — `IModuleInterface`：
- `StartupModule()`：拿 `FPropertyEditorModule`，`RegisterCustomPropertyTypeLayout("MFItemKey", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMFItemKeyCustomization::MakeInstance))`。
- `ShutdownModule()`：`UnregisterCustomPropertyTypeLayout("MFItemKey")`。
- `IMPLEMENT_MODULE(FProjectMFEditorModule, ProjectMFEditor)`。

**④ `ProjectMFEditor.Target.cs`** — `ExtraModuleNames.Add("ProjectMFEditor")`（uproject 已列则 target 自动带，此步可选做保险）。

### 6.2 Runtime：`FMFItemKey`（Inventory 模块）

`Source/ProjectMF/Inventory/Public/MFItemKey.h` + `Private/MFItemKey.cpp`：
```cpp
USTRUCT(BlueprintType)
struct PROJECTMF_API FMFItemKey
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
    int32 ItemID = 0;

    const FMFItemDef* Resolve() const;   // .cpp: UMFItemStatics::FindItem(UMFItemSettings::GetItemTable(), ItemID)
    bool  IsValid()  const { return ItemID > 0; }
    bool  operator==(const FMFItemKey& O) const { return ItemID == O.ItemID; }
    friend uint32 GetTypeHash(const FMFItemKey& K) { return ::GetTypeHash(K.ItemID); }
};
```
Resolve 放 .cpp（依赖 UMFItemStatics/UMFItemSettings，避免头文件循环）。

### 6.3 Editor：`FMFItemKeyCustomization`（下拉控件）

`Source/ProjectMFEditor/Private/MFItemKeyCustomization.h/.cpp`，`IPropertyTypeCustomization`：

- `static TSharedRef<IPropertyTypeCustomization> MakeInstance()`。
- `CustomizeHeader(PropertyHandle, HeaderRow, Utils)`：
  1. `ItemIDHandle = PropertyHandle->GetChildHandle("ItemID")`。
  2. **建选项**：遍历 `UMFItemSettings::GetItemTable()` 的 `GetRowMap()`，每行 `FindRow<FMFItemDef>` → 组 `"DisplayName (#RowName)"`，同时建 `TMap<FString 显示, int32 ItemID>` 和反向 `TMap<int32, TSharedPtr<FString>>`。缓存进成员（物品变动时可 `ForceRefresh`）。
  3. **控件**：`SSearchableComboBox`（可搜索，物品多友好），`OptionsSource = &Options`，`OnSelectionChanged` → 从选中显示串查 ItemID → `ItemIDHandle->SetValue(ItemID)`；`OnGenerateWidget` 出 `STextBlock`。
  4. **当前值显示**：`ItemIDHandle->GetValue(Cur)` → 反查显示串（查不到显示 `"#ID (缺失)"` 提示配置错误）。
- `CustomizeChildren`：空实现（header 一行下拉够用；不想暴露裸 ItemID 就不加子行）。

选项数据源直接读 DT_Item，不建 F1 那样的 Trait 缓存层——ProjectMF 物品量小，编辑器里现读即可；要优化再加缓存 + `OnDataTableChanged` 刷新。

### 6.4 引用处：int32 → FMFItemKey

| 位置 | 改动 |
|---|---|
| `FMFLootEntry.ItemID`（MFLootTypes.h） | → `FMFItemKey Item` |
| `MFLootSubsystem::RollTable` | `Entry.ItemID` → `Entry.Item.ItemID`（Result 仍存 int32，不改 FMFLootResult） |
| 配方 `FMFRecipeDef`（A1.8 建时） | Inputs/Output 直接用 FMFItemKey |
| **不动** | `MFSpawnLoot` exec 参数、背包 `FMFInventorySlot.ItemID`、`FMFLootResult.ItemID`——运行时数据/调试，非配置面 |

### 6.5 实现顺序

| 步 | 内容 | 验证 |
|---|---|---|
| 1 | 建 ProjectMFEditor 模块（uproject + Build.cs + Module 类，StartupModule 先空注册） | 编译过，编辑器加载模块 |
| 2 | `FMFItemKey`（runtime，含 Resolve） | 编译过 |
| 3 | `FMFItemKeyCustomization` + 注册 | DT 里某处放个 FMFItemKey 字段 → 下拉出现、能选、显示物品名 |
| 4 | `FMFLootEntry.ItemID` → `FMFItemKey` + RollTable 取 `.ItemID` | LT_Test 掉落表 Entry 变下拉；`MFDropTable` 仍正常掉落 |

**验收**：编辑 LT_Test 时，物品那栏是**下拉框**，列出所有物品的 "名字 (#ID)"，可搜索、点选；运行时掉落/roll 行为不变。

### 6.6 注意点

- **编译方式变化**：新增编辑器模块后，第一次要让 UBT 重新生成项目（`ProjectMFEditor` target 会带上新模块）；编辑器需重启加载新模块。
- **CDO/热重载**：新增 UCLASS/USTRUCT 同样需重启编辑器，Live Coding 对新模块不可靠。
- **模块加载相位**：customization 注册在 `StartupModule`；Editor 模块 `Default` 相位足够（PropertyEditor 已在）。
