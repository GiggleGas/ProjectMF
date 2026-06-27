//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/DungeonEventListener.h"

#include "Core/Dungeon.h"

#include "EngineUtils.h"

DEFINE_LOG_CATEGORY(DungeonEventListenerLog);


UDungeonEventListener::UDungeonEventListener(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {

}

void UDungeonEventListener::OnPreDungeonBuild_Implementation(ADungeon* Dungeon) {

}

void UDungeonEventListener::OnDungeonLayoutBuilt_Implementation(ADungeon* Dungeon) {

}

bool UDungeonEventListener::OnMarkersEmitted_Implementation(ADungeon* Dungeon,
                                                            UPARAM(ref) TArray<FDungeonMarkerBuildData>& MarkerList,
                                                            TArray<FDungeonMarkerBuildData>& NewMarkerList) {
    return false;
}

void UDungeonEventListener::OnPostDungeonBuild_Implementation(ADungeon* Dungeon) {

}

void UDungeonEventListener::OnPreDungeonDestroy_Implementation(ADungeon* Dungeon) {

}

void UDungeonEventListener::OnDungeonDestroyed_Implementation(ADungeon* Dungeon) {

}

////////////////////////////////// FDungeonEventListenerNotifier ////////////////////////////////// 

void DungeonUtils::FDungeonEventListenerNotifier::GetInterfaceImplementers(ADungeon* Dungeon, TArray<UObject*>& OutImplementers) {
    OutImplementers.Reset();
    if (!Dungeon) return;

    TArray<UActorComponent*> Components;
    Dungeon->GetComponents(Components);
    for (UActorComponent* Component : Components) {
        if (Component && Component->Implements<UDungeonEventListenerInterface>()) {
            OutImplementers.Add(Component);
        }
    }
    
    /*
    UWorld* World = Dungeon->GetWorld();
    if (!World) return;

    // Find all actors implementing the interface
    for (TActorIterator<AActor> It(World); It; ++It) {
        AActor* Actor = *It;
        if (Actor && Actor->Implements<UDungeonEventListenerInterface>()) {
            OutImplementers.Add(Actor);
        }

        // Also check components on the actor
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components) {
            if (Component && Component->Implements<UDungeonEventListenerInterface>()) {
                OutImplementers.Add(Component);
            }
        }
    }
    */
}

void DungeonUtils::FDungeonEventListenerNotifier::NotifyPreDungeonBuild(ADungeon* Dungeon) {
    if (!Dungeon) return;
    for (UDungeonEventListener* Listener : Dungeon->EventListeners) {
        if (!Listener) continue;
        Listener->OnPreDungeonBuild(Dungeon);
    }

    // Notify interface implementers
    TArray<UObject*> Implementers;
    GetInterfaceImplementers(Dungeon, Implementers);
    for (UObject* Implementer : Implementers) {
        IDungeonEventListenerInterface::Execute_OnPreDungeonBuild(Implementer, Dungeon);
    }
}

void DungeonUtils::FDungeonEventListenerNotifier::NotifyDungeonLayoutBuilt(ADungeon* Dungeon) {
    if (!Dungeon) return;
    for (UDungeonEventListener* Listener : Dungeon->EventListeners) {
        if (!Listener) continue;
        Listener->OnDungeonLayoutBuilt(Dungeon);
    }

    // Notify interface implementers
    TArray<UObject*> Implementers;
    GetInterfaceImplementers(Dungeon, Implementers);
    for (UObject* Implementer : Implementers) {
        IDungeonEventListenerInterface::Execute_OnDungeonLayoutBuilt(Implementer, Dungeon);
    }
}

void DungeonUtils::FDungeonEventListenerNotifier::NotifyMarkersEmitted(ADungeon* Dungeon,  TArray<FDungeonMarkerBuildData>& Markers) {
    if (!Dungeon) return;
    for (UDungeonEventListener* Listener : Dungeon->EventListeners) {
        if (!Listener) continue;
        TArray<FDungeonMarkerBuildData> NewMarkers;
        if (Listener->OnMarkersEmitted(Dungeon, Markers, NewMarkers)) {
            Markers = NewMarkers;
        }
    }

    // Notify interface implementers
    TArray<UObject*> Implementers;
    GetInterfaceImplementers(Dungeon, Implementers);
    for (UObject* Implementer : Implementers) {
        TArray<FDungeonMarkerBuildData> NewMarkers;
        if (IDungeonEventListenerInterface::Execute_OnMarkersEmitted(Implementer, Dungeon, Markers, NewMarkers)) {
            Markers = NewMarkers;
        }
    }
}

void DungeonUtils::FDungeonEventListenerNotifier::NotifyPostDungeonBuild(ADungeon* Dungeon) {
    if (!Dungeon) return;
    for (UDungeonEventListener* Listener : Dungeon->EventListeners) {
        if (!Listener) continue;
        Listener->OnPostDungeonBuild(Dungeon);
    }

    // Notify interface implementers
    TArray<UObject*> Implementers;
    GetInterfaceImplementers(Dungeon, Implementers);
    for (UObject* Implementer : Implementers) {
        IDungeonEventListenerInterface::Execute_OnPostDungeonBuild(Implementer, Dungeon);
    }
}

void DungeonUtils::FDungeonEventListenerNotifier::NotifyPreDungeonDestroy(ADungeon* Dungeon) {
    if (!Dungeon) return;
    for (UDungeonEventListener* Listener : Dungeon->EventListeners) {
        if (!Listener) continue;
        Listener->OnPreDungeonDestroy(Dungeon);
    }

    // Notify interface implementers
    TArray<UObject*> Implementers;
    GetInterfaceImplementers(Dungeon, Implementers);
    for (UObject* Implementer : Implementers) {
        IDungeonEventListenerInterface::Execute_OnPreDungeonDestroy(Implementer, Dungeon);
    }
}

void DungeonUtils::FDungeonEventListenerNotifier::NotifyDungeonDestroyed(ADungeon* Dungeon) {
    if (!Dungeon) return;
    for (UDungeonEventListener* Listener : Dungeon->EventListeners) {
        if (!Listener) continue;
        Listener->OnDungeonDestroyed(Dungeon);
    }

    // Notify interface implementers
    TArray<UObject*> Implementers;
    GetInterfaceImplementers(Dungeon, Implementers);
    for (UObject* Implementer : Implementers) {
        IDungeonEventListenerInterface::Execute_OnDungeonDestroyed(Implementer, Dungeon);
    }
}

