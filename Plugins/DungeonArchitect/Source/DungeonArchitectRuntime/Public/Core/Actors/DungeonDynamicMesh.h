//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "DungeonDynamicMesh.generated.h"

UCLASS(hidecategories = (LOD), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class DUNGEONARCHITECTRUNTIME_API UDungeonDynamicMeshComponent : public UDynamicMeshComponent {
	GENERATED_UCLASS_BODY()
	
protected:
	virtual void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup) override;
};

UCLASS(ConversionRoot, ComponentWrapperClass, ClassGroup=DynamicMesh, meta = (ChildCanTick))
class ADungeonDynamicMesh : public AActor {
	GENERATED_UCLASS_BODY()
	
protected:
	UPROPERTY(Category = DynamicMeshActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|StaticMesh", AllowPrivateAccess = "true"))
	TObjectPtr<class UDungeonDynamicMeshComponent> DynamicMeshComponent;

public:
	UFUNCTION(BlueprintCallable, Category = DynamicMeshActor)
	UDungeonDynamicMeshComponent* GetDynamicMeshComponent() const { return DynamicMeshComponent; }
	
};

