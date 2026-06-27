//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonEventListener.h"

#include "Curves/CurveFloat.h"
#include "Landscape.h"
#include "DungeonLandscapeTransformer.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLandscapeModifier, Log, All);

UENUM(BlueprintType)
enum class ELandscapeFalloffEasing : uint8
{
	Linear			UMETA(DisplayName = "Linear"),
	EaseIn			UMETA(DisplayName = "Ease In"),
	EaseOut			UMETA(DisplayName = "Ease Out"),
	EaseInOut		UMETA(DisplayName = "Ease In-Out"),
	SmoothStep		UMETA(DisplayName = "Smooth Step"),
	SmootherStep	UMETA(DisplayName = "Smoother Step")
};

UENUM(BlueprintType)
enum class EWeightPaintBlurStrength : uint8
{
	None	UMETA(DisplayName = "None"),
	Light	UMETA(DisplayName = "Light (2 passes)"),
	Medium	UMETA(DisplayName = "Medium (4 passes)"),
	Heavy	UMETA(DisplayName = "Heavy (6 passes)")
};

UENUM(BlueprintType)
enum class EWeightPaintBlurSize : uint8
{
	Small	UMETA(DisplayName = "Small (3x3)"),
	Medium	UMETA(DisplayName = "Medium (5x5)"),
	Large	UMETA(DisplayName = "Large (7x7)")
};

class ULandscapeEditLayerDungeonArchitect;
class FDungeonLandscapeDataRasterizer;
class ULandscapeLayerInfoObject;
struct FLandscapeEditDataInterface;

// Base class for SDF-based paint layer configuration
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType)
class DUNGEONARCHITECTRUNTIME_API ULandscapePaintSDFConfig : public UObject
{
	GENERATED_BODY()
public:
	virtual void GetSDFRange(float& OutMinSDF, float& OutMaxSDF) const PURE_VIRTUAL(ULandscapePaintSDFConfig::GetSDFRange, );
};

// Fills the dungeon interior (SDF < 0)
UCLASS(DisplayName = "Fill Interior")
class DUNGEONARCHITECTRUNTIME_API ULandscapePaintSDFConfig_FillInterior : public ULandscapePaintSDFConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon, meta = (ClampMin = "0.0"))
	float BoundaryInset = 0.0f;

	virtual void GetSDFRange(float& OutMinSDF, float& OutMaxSDF) const override {
		OutMinSDF = -1000000.0f;
		OutMaxSDF = -BoundaryInset;
	}
};

// Fills outside the dungeon (SDF > 0)
UCLASS(DisplayName = "Fill Exterior")
class DUNGEONARCHITECTRUNTIME_API ULandscapePaintSDFConfig_FillExterior : public ULandscapePaintSDFConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon, meta = (ClampMin = "0.0"))
	float MaxDistance = 1000.0f;

	virtual void GetSDFRange(float& OutMinSDF, float& OutMaxSDF) const override {
		OutMinSDF = 0.0f;
		OutMaxSDF = MaxDistance;
	}
};

// Fills the border region (around SDF = 0)
UCLASS(DisplayName = "Fill Border")
class DUNGEONARCHITECTRUNTIME_API ULandscapePaintSDFConfig_FillBorder : public ULandscapePaintSDFConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon, meta = (ClampMin = "0.0"))
	float InnerWidth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon, meta = (ClampMin = "0.0"))
	float OuterWidth = 100.0f;

	virtual void GetSDFRange(float& OutMinSDF, float& OutMaxSDF) const override {
		OutMinSDF = -InnerWidth;
		OutMaxSDF = OuterWidth;
	}
};

// Advanced: raw SDF range
UCLASS(DisplayName = "Custom Range")
class DUNGEONARCHITECTRUNTIME_API ULandscapePaintSDFConfig_CustomRange : public ULandscapePaintSDFConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon)
	float MinSDFDistance = -100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon)
	float MaxSDFDistance = 0.0f;

	virtual void GetSDFRange(float& OutMinSDF, float& OutMaxSDF) const override {
		OutMinSDF = MinSDFDistance;
		OutMaxSDF = MaxSDFDistance;
	}
};

// Paint layer entry with layer info and SDF config
USTRUCT(BlueprintType)
struct FLandscapePaintLayerEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon)
	TObjectPtr<ULandscapeLayerInfoObject> LayerInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon, Instanced)
	TObjectPtr<ULandscapePaintSDFConfig> SDFConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon, meta = (ClampMin = "0", ClampMax = "255"))
	uint8 Weight = 255;

	// Blur settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blur")
	EWeightPaintBlurStrength BlurStrength = EWeightPaintBlurStrength::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blur")
	EWeightPaintBlurSize BlurSize = EWeightPaintBlurSize::Medium;

	// Noise settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	bool bEnableNoise = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (EditCondition = "bEnableNoise", ClampMin = "0.0"))
	float NoiseScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (EditCondition = "bEnableNoise", ClampMin = "0.0", ClampMax = "1.0"))
	float NoiseAmplitude = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (EditCondition = "bEnableNoise", ClampMin = "1", ClampMax = "8"))
	int32 NoiseOctaves = 3;
};

class UGridDungeonModel;
class UGridDungeonConfig;
class USimpleCityModel;
class USimpleCityConfig;

/**
* Modifies the landscape.  Attach to the dungeon actor
*/
UCLASS(Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent), hidecategories = (Rendering,Input,Actor,Misc,Replication,Collision,LOD,Cooking,HLOD,Physics,Networking,Tags,Activation,AssetUserData,Navigation))
class DUNGEONARCHITECTRUNTIME_API UDungeonLandscapeTransformer : public UActorComponent, public IDungeonEventListenerInterface {
    GENERATED_BODY()

public:
    UDungeonLandscapeTransformer(const FObjectInitializer& ObjectInitializer);

	virtual void OnDungeonLayoutBuilt_Implementation(ADungeon* Dungeon) override;
	virtual void OnPreDungeonDestroy_Implementation(ADungeon* Dungeon) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	
    //void OnDungeonLayoutBuilt(ADungeon* Dungeon);
    //void OnPreDungeonDestroy(ADungeon* Dungeon);

private:
#if WITH_EDITOR

    void BuildLandscape(ADungeon* Dungeon);
	void Cleanup() const;
	
    ULandscapeEditLayerDungeonArchitect* GetDungeonLandscapeEditLayer(ALandscape* Landscape) const;

    void RasterizeLayoutOnLayer(ADungeon* Dungeon, ULandscapeInfo* LandscapeInfo);

#endif // WITH_EDITOR

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dungeon)
    TObjectPtr<ALandscape> Landscape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Blending)
    float BlendDistanceWorldUnits = 5000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Blending)
    ELandscapeFalloffEasing FalloffEasing = ELandscapeFalloffEasing::SmoothStep;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Blending, meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float FalloffExponent = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Blending)
    TObjectPtr<UCurveFloat> FalloffCurve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noise)
    bool bEnableFalloffNoise = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noise, meta = (EditCondition = "bEnableFalloffNoise"))
    float FalloffNoiseScale = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noise, meta = (EditCondition = "bEnableFalloffNoise", ClampMin = "0.0"))
    float FalloffNoiseAmplitude = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Noise, meta = (EditCondition = "bEnableFalloffNoise", ClampMin = "1", ClampMax = "8"))
    int32 FalloffNoiseOctaves = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Blur, meta = (ClampMin = "1", ClampMax = "8"))
    int32 BoundaryHeightBlurRadius = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Blur, meta = (ClampMin = "0.0"))
    float BoundaryHeightBlurDistance = 200;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weight Painting")
    TArray<FLandscapePaintLayerEntry> PaintLayers;
};

