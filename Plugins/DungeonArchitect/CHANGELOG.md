CHANGELOG: Dungeon Architect
============================
Version 3.5.1 [Release Notes](https://blog.dungeonarchitect.dev/release-notes-version-3-5-0/) [24-Mar-2026]
-------------
* CRITICAL FIX: Fab deployment excluded the Shaders folder from the plugin package, causing projects to crash on launch

Version 3.5.0 [Release Notes](https://blog.dungeonarchitect.dev/release-notes-version-3-5-0/) [23-Mar-2026]
-------------
* New: Build System option to disable automatic player restart, giving more control over manual in-game dungeon generation
* Fix: Voxel Dungeons now properly generate runtime Navigation
* Fix: Grid Builder's dungeons were getting cropped when using the Pattern Matcher due to a memory reference issue
* Fix: Memory crash in the Grid Builder's stair generator
* Fix: Compile errors in Lyra
* Fix: Minimap crash on dedicated servers

Version 3.4.1 [23-Jan-2026]
-------------
* Fix: Previous update didn't deploy correctly,   redeployed to fab again

Version 3.4.0 [Release Notes](https://blog.dungeonarchitect.dev/release-notes-version-3-4-0/) [22-Jan-2026]
-------------
* New: Landscape Transformer - Non-destructive landscape modifications using edit layers - seamlessly blend dungeon layouts with existing terrain and paint material layers based on distance from dungeon boundaries
* New: Canvas Minimap's LayoutDrawPercent parameter has been moved from the component to the Canvas Minimap Theme assets, since this is art specific configuration and doesn't need to be set everytime in the actors
* Fix: Grid Builder's Pattern matcher was not working correctly when the dungeon was not in the origin.    This also fixes the cropping issues at the edge of the grid builder dungeons
* Fix: Recompiled all the sample theme files to upgrade them to the lastest version.     If you have an older theme file from a year ago, open it atleast once in the theme editor and resave to upgrade.

Version 3.3.0 [Release Notes](https://blog.dungeonarchitect.dev/release-notes-version-3-3-0/) [18-Dec-2025]
-------------
* New: Canvas UMG Widget - Interactive minimap widget with mouse zoom and pan
* New: Multiplayer Sample - Full screen dungeon map viewer using the new Canvas widget
* New: Multiplayer Sample - Lobby minimap preview with seed randomization, synced to all clients in real-time
* New: BuildDungeonWithSettings node - Specify start and end build phases to control dungeon generation. Build only up to the layout phase to preview in the Canvas widget without spawning meshes
* Fix: CellFlow Builder - Canvas minimap was rendering some doors as walls, blocking fog of war exploration until the player moved past them
* Fix: Canvas Theme Editor - Improved stability. Reordering, adding, and removing layers now correctly retains layer values
* Fix: Code Compile errors while packaging has been fixed
* Change: Build System Component - RandomizeOnBuild now defaults to false. Enable for random seeds each session, or keep disabled to control seed manually via Blueprints (as in the Multiplayer demo)

Version 3.2.0 [Release Notes](https://blog.dungeonarchitect.dev/release-notes-version-3-2-0/) [12-Dec-2025]
-------------
* NEW: Multiplayer Sample - Complete networked dungeon sample with server browser, lobby, and late-join support. The Build System coordinates dungeon generation across server and clients - handling login, instructing clients to build locally, waiting for completion (in spectator mode), then spawning players as pawns. Session management and lobby code provided as reference implementations. Available in Launch Pad.
* NEW: Build System Overhaul - Two build modes: "wait for all clients" and "direct start". Removed ADungeonGameMode in favor of components
* NEW: Navigation Support in Theme Nodes - Theme mesh nodes now has a new flag to affect navigation for both instanced and non-instanced actors/blueprints
* NEW: Dungeon Streaming - Spawn rooms are now kept as always resident to avoid race conditions for late-spawning actors
* Fix: Canvas no longer affects fog of war when using spectator pawn
* Fix: HISM meshes were not including actors spawned by the Multi Mesh theme node
* BREAKING: ADungeonGameMode has been removed - use [build system](https://docs.dungeonarchitect.dev/unreal/build-system/build-system-overview) components instead.
* BREAKING: Add a DungeonCanvas component to your Dungeon actor's if you want to use the mini-map framework.   It is no longer added by default

Version 3.1.0  [04-Dec-2025]
-------------
* CRITICAL FIX: Grid Flow Graph Editor was not showing the items on the Layout nodes 
* New: Spawn Logic support World context, so you can add components, spawn actors etc from within the spawn logic blueprint
* New: Spawn Logic's OnItemSpawn function no longer provides a Random input. Use the GetRandomFloat node instead that has an exec pin to control the state modification of the random stream in a determinisitc way

Version 3.0.4  [02-Dec-2025]
--------------
* CRITICAL FIX: Fixed a crash issue in the theme editor you click on the viewport and close the window
* Fix: Theme Editor Mode supports drag drop of blueprint actors on to the scene.  
* Fix: Theme Editor MOde selection issue in some cases where actors were not selected correclty after a async rebuild and has been fixed 

Version 3.0.3  [27-Nov-2025]
--------------
* New: Renamed "ConsumeOnAttach" flag in the theme node to "Stop on Select" to better reflect the functionality.
* Fix: Various stability and quality of life improvements to the new Dungeon Theme Editor Mode

Version 3.0.2  [26-Nov-2025]
--------------
* Fix: Theme editor and Theme EditorMode were not spawning default marker nodes when the builder class was changed in the settings
* Fix: Theme Editor Mode wasn't initializing the EdMode's graph editor correctly for newly created assets.   They had to be opened atleast once in the theme editor window before use.
* Fix: Errors while packaging the project have been fixed

Version 3.0.1  [21-Nov-2025]
--------------
* CRITICAL FIX: The new Launch Pad Sample browser window was not showing up correctly

Version 3.0.0 [Release Notes](https://blog.dungeonarchitect.dev/release-notes-version-3-0-0/) [20-Nov-2025]
--------------
* New: GPU Voxel Engine - Generate infinite organic worlds (Caves, Floating Islands) using GPU compute shaders with full Nanite & Lumen support
* New: Interactive Theme Editor - Design themes directly in the Level Viewport using drag-and-drop workflows with real-time propagation
* New: Cell Flow Builder - New Voronoi-based layout generator for organic, non-grid dungeon structures with dynamic height support
* New: PCG Framework Integration - Use Flow Graphs to drive Unreal's PCG system (includes Electric Dreams integration samples)
* New: "Foundry" Theme - A production-ready Industrial Sci-Fi theme asset pack included for free
* New: Launch Pad Dashboard - Completely rewritten web-based dashboard for one-click access to samples and documentation
* New: Unreal Engine 5.7 Support
* New: Pattern Matcher improvements including new "Copy Markers" and "Rename Markers" nodes
* Fix: Optimized scene provider to limit active latent tasks, preventing GPU overloads during heavy generation

Version 2.38.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-37-2) [04-Jun-2025]
--------------
* New: Unreal Engine 5.6 Support

Version 2.37.3 [Release Notes](https://blog.dungeonarchitect.dev/release-2-37-2) [25-Apr-2025]
--------------
* CRITICAL FIX: Marker Emitter blueprints were not registered correctly with the theme engine and were not getting executed
* CRITICAL FIX: Theme Assets created before version `2.37.0` were not auto-upgraded correctly, which caused them to not generate the dungeon in standalone builds, and has been fixed
* New: Lots of improvements and optimizations on the theme engine code

Version 2.37.2 [Release Notes](https://blog.dungeonarchitect.dev/release-2-37-2) [21-Mar-2025]
--------------
* CRITICAL FIX: Dungeon volumes (Theme Override, Negation, Platform etc.) now work properly after fixing incorrect bounds calculations

Version 2.37.1 [Release Notes](https://blog.dungeonarchitect.dev/release-2-37-1) [19-Mar-2025]
--------------
* CRITICAL FIX: Fixed a crash that occurred when building SGF dungeons

Version 2.37.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-37-0) [18-Mar-2025]
--------------
* CRITICAL FIX: Resolved packaging and standalone build failures caused by Engine Upgrade from `5.5.3` to `5.5.4`
* New: Theme Editor Mode - a new and powerful way to design your dungeon themes, directly within the level viewport
* New: Enhanced Theme Graph hierarchy - Mesh nodes can now emit other mesh nodes to create complex structures, replacing previous MarkerEmitter node limitations
* New: Grid Dungeons now support full positional and rotational control - place and rotate your dungeons by adjusting the dungeon actor's transform
* New: Removed Legacy Spatial constraints in favor of the more powerful Pattern Matcher system
* New: Added 'Build' button to Theme Editor window for manual preview viewport refreshing
* Fix: Improved Theme Editor performance with thumbnail caching, eliminating flickering when updating theme graphs
* Fix: Fixed cleanup issues when rebuilding dungeons after modifying the "Instanced" flag

Version 2.36.1 [Release Notes](https://blog.dungeonarchitect.dev/release-2-36-1) [27-Dec-2024]
--------------
* CRITICAL FIX: Canvas Editor - Opening newly created dungeon canvas assets caused editor crashes
* CRITICAL FIX: Theme Editor - Modifying config properties after changing builder type resulted in editor crashes

Version 2.36.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-36-0) [25-Dec-2024]
--------------
* CRITICAL FIX: Theme editor viewport property changes no longer trigger editor crashes
* Fix: Resolved multiplayer build sequence issue where dungeons were building on clients before server completion
* Fix: Corrected Grid Flow debug visualization alignment for dungeons placed away from world origin
* Fix: Improved memory management during dungeon generation to prevent editor crashes when changing levels while the dungeon was being built
* Fix: Resolved naming conflicts between Lyra and Dungeon Architect Gameplay framework (DAG)
* New: SnapMap editor has a Result Graph Node space settings to customize the amount of spacing between the node layout

Version 2.35.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-35-0) [14-Nov-2024]
--------------
* New: Unreal Engine 5.5 support
* New: Content restructuring - Relocated legacy samples from `Content/Samples` to `Content/Showcase/Legacy/Samples` as part of ongoing content modernization effort 
* New: Flow graph items now support metadata, including a weight value that indicates spawn difficulty along paths
* Fix: Snap Editor no longer crashes when initializing template objects in certain cases
* Fix: Dungeon build system was incorrectly initiating the build on clients when auto-build flag was turned off
* Fix: Build System - Prevented unwanted client-side dungeon generation when server auto-build is disabled

Version 2.34.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-34-0) [02-Nov-2024]
--------------
* EXPERIMENTAL: PCG integration with Dungeon Architect (UE 5.4+) - New PCG nodes for dungeon layout and SDF texture access, with theme editor integration, enabling advanced theming possibilities
* New: Build System can be used without modifying the GameMode's class hierarchy.  Attach a component to your existing game mode instead
* New: DungeonCanvas minimap materials support transparency with fog of war
* New: Dungeon actor introduces Canvas flag `GenerateTexturesOnDemand` (enabled by default). This flag defers resource allocation (e.g., fog of war textures) until needed. Disable to allocate and update resources immediately upon dungeon creation
* New: Dungeon Progress Status - Captures build state for loading screens, provides queryable text and progress info, and shows real-time progress bar in editor during async building
* New: Dependency execution graph for optimized dungeon build management - Breaks complex builds into smaller tasks with defined dependencies, enabling efficient async execution (e.g., PCG graphs) in the correct order
* New: DungeonCanvas material editor toolbar enhancements - Added preview dungeon builder selection dropdown (Grid, SGF, etc.) and a randomize button to generate new dungeon layouts in the preview viewport
* New: SGF City Samples - Generate large beautiful cities using a combination of SGF builder and Simple City builder. `Samples/DA_SGF_CitySample/SGF_City` [Video](https://www.youtube.com/watch?v=Do1HDzAkQZQ)
* CRITICAL FIX: Resolved packaging issues affecting Dungeon Canvas - Fixed incorrect material packaging that were causing minimap icon rendering errors, and incorrect theme packaging leading to crashes in standalone builds
* CRITICAL FIX: Resolved standalone build failures caused by Dungeon Canvas theme's editor-only objects
* BREAKING CHANGE: `IsNearMarker` blueprint node for GridQuery and GridFlowQuery now requires `DungeonModel` pin instead of `DungeonBuilder` pin
* Fix: Snap-based level were crashing the editor when they were rebuilt without destroying the existing dungeon
* Fix: Snap map doors spawn in the correct orientation, aligned with the flow graph.  The sample level's doors have a debug arrow next to the doors, indicating the orientation
* Fix: Snap-based level streaming now correctly restores the state of in-game hidden actors when chunks are streamed back into the world
* Fix: SnapMap Debug Draw now correctly renders module bounds and connection locations
* Fix: Snap modules now spawn into the world even if they are being blocked by something in the way
* Fix: Dungeon Canvas Editor - Optimized performance for lag-free parameter adjustments and instant preview updates
* Fix: Dungeon Canvas Editor - Fixed a bug where hiding layers would incorrectly apply hidden layer values to other layers
* Fix: Dungeon Canvas - New Water Color layer effect
* Fix: Theme editor was crashing in some cases, while changing the mesh node properties
* Fix: Theme-based dungeons now correctly use `Build Location Priority` to start construction from player location and expand outward
* Fix: Upgraded all UObject* UPROPERTY references to TObjectPtr for UE 5.5 compatibility

Version 2.33.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-33-0) [13-Sep-2024]
--------------
* New: Dungeon Canvas Framework has been rewritten to be more powerful and flexible.  It is faster, takes up less memory, support for different camera types, minimap materials that can be applied to the world, multiple widgets can share the same dungeon canvas resources with different themes and a lot more.    This also paves the way for future PCG integration
* New: Updated documentation on [Dungeon Canvas](https://docs.dungeonarchitect.dev/unreal/canvas-overview.html)
* New: Automated Dungeon Build System to automatically build your dungeons at runtime (also works on multiplayer).  Control this from the Dungeon actor's detail panel.  This avoids a lot of boilerplate blueprint code
* New: SGF supports Entrance / Exit connection constraints
* New: Grid Dungeon Platform Volumes optionally clamp the Z value of the generated dungeons.  These are great if you want to align the height of the platforms with existing level geometry
* New: Blueprint function `GetAllDungeonSpawnedActors` to fetch all the dungeon spawned actors in the scene
* Fix: SGF Door actor correctly orients itself to the direction of the flow graph link its on.  This was inconsistent in the past and the door actor would orient itself in the opposite direction, in some cases.
* Fix: SGF Negation Volumes were not working correctly with the reverse flag
* Fix: SGF `Create Path` node was ignoring the Node Creation constraint
* Fix: Dungeon Canvas Room Shape Texture assets use SoftObject pointers to avoid cooking issues
* Fix: SGF ModuleDB compilation crashed the editor in some cases

Version 2.32.5 [Release Notes](https://blog.dungeonarchitect.dev/release-2-32-5) [16-Jun-2024]
--------------
* Critical Fix: Dungeon Canvas was causing standalone builds to fail while cooking
* Fix: Fixed various code warnings while cooking
* Fix: SGF Module database no longer crashes if the connection actor was placed to close to the bottom of the chunk (with door offset set to 0)
* Fix: SGF Module database uses a soft object pointer to reference the module bounds assets, to avoid cooking issues in stand alone builds

Version 2.32.4 [Release Notes](https://blog.dungeonarchitect.dev/release-2-32-4) [26-May-2024]
--------------
* Critical Fix: Rebuilding the SnapMap dungeons was crashing the editor, if any one of the room chunks were previously selected in the editor
* Fix: SGF dungeons replicate their actors correctly.   So if you place a replicated turret actor in a room chunk, it would replicate correctly on all the clients
* Fix: SGF FPS flow graph sample content had it's Start / End room category incorrectly set, which was not spawning the player correctly in the spawn room
* Fix: Multiple Snap based dungeons in the same map were breaking replication.    Implemented better hashing to avoid module ID collision
* Fix: SGF Dungeon Layout bounds returned by the Dungeon Model didn't calculate the dungeon's upper bound correctly

Version 2.32.3 [Release Notes](https://blog.dungeonarchitect.dev/release-2-32-3) [23-May-2024]
--------------
* Critical Fix: The `Config` folder was not deployed correctly in the last update, which caused blueprint errors in the samples, and some assets were not being recognized correctly.
* Critical Fix: Dungeon Canvas was crashing the editor when exiting out of a map with world partition enabled
* Fix: Actors tagged as `HiddenInGame` were incorrectly made visible by the Level Streaming system when it came back to view.   The original `HiddenInGame` flag is now retained correctly, while making the room chunk visible.
* Fix: The `Spawn Items` node was sometimes spawning `(MaxCount + 1)` items due to a rounding error.  The value is now clamped to Min/Max
* Fix: The `Spawn Items` node in the flow framework uses the `SpawnDistributionVariance` correctly
* New: SGF Item fitness calculation has an early bailout system so it doesn't take too like if it cannot find a module with enough placeable markers to spawn the items.    This makes the dungeon build faster
* New: Access the generated dungeon Bounds from the Dungeon Model object.   `Dungeon Actor > Get Dungeon Model > Get Dungeon Bounds`

Version 2.32.2 [Release Notes](https://blog.dungeonarchitect.dev/release-2-32-2) [21-May-2024]
--------------
* Critical Fix: Snap and SGF modules level files could not be saved due to an editor only object inside the Connection actor
* Fix: Crash issues when the Level streaming model accesses stale level module pointers

Version 2.32.1 [Release Notes](https://blog.dungeonarchitect.dev/release-2-32-1) [18-May-2024]
--------------
* Fix: SGF algorithm performance improvements.  Rewrote the bin packing algorithms used for module selection and is no longer slow in some cases when a large number of items are spawned

Version 2.32.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-32-0) [10-May-2024]
--------------
* New: Unreal Engine 5.4 support
* New: Dungeon Canvas - A powerful new dungeon minimap framework, with fog of war, visibility, multiple floor support and much more.  Find more info in the release notes.
* New: Level streaming code has been upgraded to use UE5's Level Instance feature. This makes the streaming more stable and efficient.   This upgrade propagates to `Snap Grid Flow` and `Snap Map` dungeon builders
* New: Removed Snap Persistent dungeons flag, as they are not needed anymore with the new Level Instance feature.  You can now save your snap dungeons that were generated in the editor.
* New: Grid Builder's system system rewrite.  It generates high quality results without any artifacts.  Please refer the docs for more info
* New: Old Dungeon minimaps have been removed and will be superseded by Dungeon Canvas.   Dungeon Canvas minimap demos are available for all builders (Dungeon Architect Content > DA_Canvas_Demo)
* New: Negation volume works with SGF builder and GridFlow builders.  Use this to control the growth of your procedural dungeons.  This can be used to blend it with your exist static world geometry
* EXPERIMENTAL: Supply different themed snap modules in the module database. This allows you to create different themed dungeons.  These are great for creating simplified low-res 3d minimaps of the dungeon,   or you may switch between two timezones,  futuristic / medieval dungeon with different art styles
* New: New documentation [website]( https://docs.dungeonarchitect.dev/unreal)
* Fix: Renamed the C++ structures `FCell` and `FCellDoor` to a namespaced prefix like `FGridDungeonCell`, so they don't collide with other plugins that might use the same name
* Fix: SnapFlow and Flow graph assets are exposed to blueprints
* Fix: SGF flow graph `Create Path` node had an incorrect category assigned in the last node in certain cases
* New: Grid builder's Clustered Theming setting has it's height variation flag set to false
* New: PlaceableMarker's C++ function GetAllowedMarkerNames has been made virtual for extensibility

Version 2.31.1 [Release Notes](https://blog.dungeonarchitect.dev/release-2-31-1) [25-Oct-2023]
--------------
* Critical Fix: Flow Graph samples were crashing the engine due to deployment issues on the previous build (missing Config folder in the plugin directory)
* New: Grid Flow Transform logic blueprints have the marker transform as input

Version 2.31.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-31-0) [14-Oct-2023]
--------------
* New: Snap Connection Selector Blueprints - Apply this blueprint on your snap connection theme graphs. These blueprints give you access to the two room category it connects to and are great for selecting special doors for certain rooms (like boss / treasure rooms)
* New: Dungeon Streaming Events - Any actor inside the snap modules can listen to dungeon chunk streaming events (loaded, unloaded, visible and hidden) by implementing the interface `DungeonStreamingChunkEventInterface`
* New: Dungeon Streaming Actor Serialization - Save the state of any actor in a snap chunk while loading / unloading by implementing the interface `DungeonStreamingActorSerialization`.   Check the `SaveGame` flag on any variable that needs serialization
* New: SGF Procedural fill sample.  Create only the main rooms (like boss, treasure, shop and spawn) and have the rest of the space in between procedurally generated
* New: Run Marker Emitter blueprints either before or after the pattern matcher, thereby controlling access to the marker on the other system. If you want the marker emitter blueprint to have access to the pattern matcher emitted markers, run it after the pattern matcher executes
* New: SGF Module Selector blueprints allow you to select or filter out the modules before being placed.  E.g. Add (or don't add) the pillar variation of the room depending on whether there's another room on top of it
* New: Snap Map builder supports module rotation on individual modules (similar to SGF), instead of having a single global flag to allow / disallow rotation of modules
* New: Node Coordinate input has been added to the Flow Graph Module Category selection blueprint.  This allows you to decorate your dungeons based on where they are (e.g. a different look on each floor, or a different room type on the edges)
* New: Grid Flow Query has a new node to return connected chunk node coords, to allow traversal across the flow graph
* New: SGF sample that uses the new module selector to add pillars if there's another module on top of it
* New: Blueprint node `DARecreateActorInLevel` to recreate actors from snap module level to the persistent level.     If you have dynamic actors that are part of a snap module (e.g. a weapon that you can pick up or an NPC), it would disappear when the level streams out.  Use this node to recreate the actor in the persistent level
* Fix: Theme Engine crashed in some instances when nested markers were used on larger theme files
* Fix: Landscape Modifier class's properties are exposed to blueprint

Version 2.30.0 [Release Notes](https://blog.dungeonarchitect.dev/release-2-30-0) [08-Sep-2023]
--------------
* New: Unreal Engine 5.3 support
* New: SnapMap Custom Module Bounds Shape - Define snap module bounds using custom shapes, allowing for new adnvaced level layouts
* New: Multi-Mesh Theme Node - Drag multiple mesh assets into the theme editor to create a Multi-Mesh theme node.  A random mesh from this list would be selected
* New: Flow Graph Branch Selection - Select different paths in your flow exec graph to add randomness to your game
* New: New Minimap Framework - The Mini Map framework has ben rewritten. All builders can write to a unified 2D minimap framework.  Snap based dungeons support 3D minimaps
* New: Snap Connection Entrance and Exit Constraints. Setting a door constraint to Entrance would ensure that we do not exit the room from this door.    These are great for puzzle games
* New: Pattern Matcher rule graph has a new `MarkerListExists` node that takes in a list of markers and performs a logical OR on it.  This reduces the number of nodes you need to create
* New: Procedural Marker Emitters
* New: Procedural Transform Rules -  These are built-in rules that are used frequently.  Apply these to your nodes, without having to create them everytime
* New: SGF City Sample - A large drivable city built using the Snap Grid Flow builder
* New: Spawn Logic function has access to the dungeon's Random Stream object.  Use this for any randomness, so all clients generate the same dungeon everywhere
* New: Custom Dungeon Item Folder Name - Spawned dungeon items are placed in a certain folder. Override this folder name using the `UseCustomItemFolderName` setting
* New: Support buttons (for Discord, Forums and Docs) added to the dungeon actor details window and on the toolbars of all the editors
* New: The Builder class drop down on the dungeon actor shows a custom widget with more info about each builder
* CRITICAL FIX: Drag drop assets from the content browser on to the theme editor was not working and has been fixed
* Fix: Reduced the Samples folder size by half by optimizing the assets. This should reduce your download time
* Fix: Theme nodes's Transform rule, Selection rule and Spawn logic gets disabled if their corresponding enabled flag is unchecked
* Fix: Flow Editor's preview viewport's shadows were pitch black in 5.2+. Added a SkyLight to brighten the scene
* Fix: Reduced the download size of Dungeon Architect by half, by optimizing the sample assets
* Fix: SnapMap Dungeon Rotation / Scaling works correctly.  Transform the dungeon actor and rebuilt
* Fix: Theme editor's marker visualizer was not rendering correctly
* Fix: Fixed a bug in SGF link when connecting different paths together
* Fix: Theme editor's actor node was not setting the transform correctly
* Fix: Removed experimental tag from the fast cell distribution on the grid config. This was causing the settings to be disabled in the editor
* Fix: Some sample blueprints had compile errors and has been fixed
* Fix: Fixed a dungeon deletion bug where actors were destroyed while being traversed
* Fix: Fixed the `SGraphNode` hard reference warnings in debug mode
* Fix: Fixed a crash issue in the theme editor's procedural mesh
* Fix: Fixed compile errors on Linux
* Fix: Build / Destroy dungeon buttons set the map state to dirty so it can be saved correctly
* Fix: Dungeon Models cleanup correctly when the dungeon is destroyed

Version 2.29.0 [11-May-2023]
--------------
* New: Unreal Engine 5.2 support

Version 2.28.0 [17-Nov-2022]
--------------
* New: Unreal Engine 5.1 support
* Fix: Grid Builder's Pattern Matcher works correctly when the dungeons are spawned away from the origin
* New: Layout Graph Visualizer for Grid Flow dungeons
* New: Internal improvements to the flow framework to make it more extensible
* Fix: SGF debug layout visualizer renders correctly when the dungeon is built away from the origin

Version 2.27.1 [08-Jul-2022]
--------------
* New: City Builder correctly supports rotation. Rotate the dungeon actor to have the city orient itself along the actor's transform
* New: Documentation for [Pattern Matcher](https://docs.dungeonarchitect.dev/unreal/pattern-matcher-getting-started.html)
* New: Pattern Matcher Editor's "Layers" panel has been renamed to "Patterns". Layers are now called patterns
* Fix: Modifying the Pattern Matcher's Pattern properties in the details panel correctly auto-updates the preview viewport
* Fix: Pattern Matcher rule node's Exec pins were rendered incorrectly as normal pins (UE5)
* Fix: Pattern matcher works correctly with the city builder
* Fix: Theme editor crash fix in rare cases due to a stale pointer

Version 2.27.0 [09-Jun-2022]
--------------
* Fix: Fixed all FGCObject warnings and errors while trying to package the project
* New: Grid Flow Item placement gives you more control on the placement of the objects (random placement, edges, center etc)
* New: Grid Flow Item placement has a flag to avoid placing items (enemies, keys etc) near doors
* Fix: Grid Flow crash fix while building the dungeon at runtime.  This was caused due to a stale pointer in some cases
* Fix: Changing the Theme Editor's builder class correctly adds/removes marker nodes for that builder
* Fix: Theme Editor's pattern matcher viewport crash fix while selecting an actor in some cases

Version 2.26.0 [06-Apr-2022]
--------------
* New: Unreal Engine 5.0 Support
* New: A powerful new feature called Pattern Matcher has been added to the Theme Editor. This allows you to decorate your levels in ways that were not possible before
* New: Launch pad sample themes created using the free marketplace asset "City of Brass"

Version 2.25.4 [12-Mar-2022]
--------------
* CRITICAL FIX: SGF dungeon no longer repeat the same rooms throughout the dungeons
* Fix: SGF Non-Repeating rooms feature works correctly, use it to avoid having the same room stitched next to each other
* Fix: Support for multiple SGF dungeons in the same scene, destroying an SGF dungeon no longer affects the other dungeon
* Fix: SGF flow debug visualization renders correctly when dungeons are built away from the origin
* Fix: Removed the redundant `Supports Door Category` flag from the module database. This flag should be set in the SGF dungeon config instead
* New: Sample demoing the Non-Repeating rooms feature: `DungeonArchitect Content > Samples > DA_SnapGridFlow_RoomRepetition`

Version 2.25.3 [08-Mar-2022]
--------------
* Fix: Remove warning message dialog box on standalone builds, the previous version didn't fix this in the marketplace version

Version 2.25.2 [06-Mar-2022]
--------------
* Fix: Remove warning message dialog box on standalone builds

Version 2.25.1 [27-Feb-2022]
--------------
* CRITICAL FIX: Previous version had Launcher deployment issue. Please update to the latest version from the launcher
* Fix: SGF dungeons were creating non-deterministic results, when built for the first time

Version 2.25.0 [26-Feb-2022]
--------------
* New: Support for UE5-Preview1 added
* New: Fixed Linux compile errors
* New: Theme Editor mesh nodes have a `Use Custom Collision` flag.  The spawned mesh's collision will be overridden only when this flag is checked.   This fixes an issue where custom collision channels were overridden with default collision values
* Fix: Fixed CDO material not found warning dialog box on Standalone server builds
* Fix: Fixed all warnings related to uninitialized variables shown in the log files
* Fix: Fixed compile errors while packaging the project
* Fix: [UE5] Removed old toolbar panel from all editor windows and replaced it with the newer ones.  This removes the `Unrecognized tab` error message
* Fix: [UE5] Flow Editor's performance panel toolbar renders correctly

Version 2.24.0 [12-Feb-2022]
--------------
* New: Snap Grid Flow module stitching can now optionally attach only to other doors with the same category.   This opens up lots of possibilities for interesting level designs
* New: Refactored the Flow framework to make it more extensible for new types of dungeon builders in the future
* New: SGF dungeon config has a new flag `SupportsDoorCategory`. Use this to enable door category based stitching, which is more computationally expensive. Keep this disabled if you don't need this feature
* New: Removed EarlyAccess tag from the GridFlow and SnapGridFlow builders

Version 2.23.3 [14-Dec-2021]
--------------
* New: Support for Linux
* New: Theme engine mesh physic properties are set correctly on the spawned meshes.  The mesh theme nodes have a new proerty for physics settings
* New: SGF builder supports selector and transform logic when spawning items in the theme editor
* Fix: Theme edtior crashes in some cases while setting runtime viewport mode

Version 2.23.2 [11-Oct-2021]
--------------
* New: Experimental: SnapMap dungeons can be built entirely on the persistent level without level streaming.  This allows you to bake lightmaps and treat it like any other level. Note: Cross actor references are not supported
* Fix: ProtoTools meshes were saved in a higher unreal engine version which was causing cooking errors in 4.26 and 4.25

Version 2.23.1 [04-Oct-2021]
--------------
* New: Experimental: SGF dungeons can be built entirely on the persistent level without level streaming.  This allows you to bake lightmaps and treat it like any other level
* New: Get the GridFlow Dungeon Bounds from the query interface
* Fix: Fixed crash issue in some cases while level streaming a SGF dungeon
* Fix: Fixed crash issue when accessing a GridFlow query interface function `GetAllChunksOfType`
* Fix: Fixed crash issues on dedicated server builds when using minimap render targets

Version 2.23.0 [22-Aug-2021]
--------------
* New: Unreal Engine 4.27 Support
* Fix: The theme engine doesn't spawn replicated actors on the client
* Fix: Fixed crash issues on dedicated server builds when using minimap render targets
* Fix: Snap Grid Flow modules were rotate incorrectly in some cases
* Fix: Ending a PIE session with the snap dungeon in it was crashing the editor in some cases
* Fix: Compiles correctly on Linux
* Fix: Fixed a null pointer issue in grid flow builder debug draw function
* Fix: City landscape transformer bug fixes (wip)
* Fix: Adjusted the size of the generated debug sgf visual nodes

Version 2.22.0 [4-Mar-2021]
--------------
* New: Level Streamer has Chunk Load / Unload bind events. Use this with the snap based builder to perform actions after a module is streamed in
* New: OnDungeonBuilt event returns a success flag indicating if the dungeon was built successfully
* New: Double click a node in the theme editor to focus the 3D viewport to an actor that was spawned with this node
* New: Selecting a marker node in the theme editor (Ground, Wall, Door etc) would show where the objects needs to be placed. These are great for aligning the meshes
* Fix: Destroying snap map dungeons at runtime was crashing the engine

Version 2.21.3 [12-Feb-2021]
--------------
* Fix: Destroying a SnapMap dungeon at runtime was crashing the engine
* Fix: Building a SnapMap dungeon when a valid graph was not generated was crashing the engine
* Fix: Some theme engine static mesh attributes was not set on dungeon rebuilds

Version 2.21.2 [11-Feb-2021]
--------------
* Fix: Previous snap assets were not being auto-upgraded to the new version. Fixed the deployment configuration that contained the upgrade information

Version 2.21.1 [10-Feb-2021]
--------------
* Fix: Running Standalone games was crashing the editor

Version 2.21.0 [2-Feb-2021]
--------------
#### SnapGridFlow Builder
* New:	A powerful new builder that supports snap modules on a 3D grid flow graph. More info [here](https://docs.dungeonarchitect.dev/unreal/sgf-introduction.html)
* New: Side Scroller Game sample using the new SnapGridFlow builder

#### Theme Editor
* New:	Added support for comment nodes in the Theme Editor
* New:	Moved the theme engine related files to the framework folder
* Fix:	Fixed a crash issue in the theme engine in some rare cases

#### Grid Flow
* New:	Rewrote the key-lock system for the flow editor. It produces better quality results
* New:	Implemented teleporters in the CreatePath node.  It works with the key-lock system
* New:	Blueprint logic controls the creation of the nodes in the flow layout graph
* Fix:	Fixed key-locks in grid flow when walls as edges option was used
* Fix:	Fixed a flow abstract graph connection bug with the sink and head nodes were the same

#### SnapMap Builder
* New:	Snap Connection Editor has been redesigned to have its own Theme editor for the doors / walls
* New:	Snap map has an option to stream in the Spawn Room first. This helps with gameplay as it would also stream in the PlayerStart actor
* New:	Improved the UI/UX of the Snap Map Editor
* New:	Added colors to the snap nodes
* New:	Removed the old blueprint based Snap builder. This has been deprecated for the past 2 years and has been replaced with the more advanced SnapMap builder
* Fix:	BSP Geometry works correctly on snap modules
* Fix:	Snap Connection use the Category value for stitching modules with similar connections, instead of the asset reference
* Fix:	Snap Editor Performance Analyzer UI bug fixes

#### Level Streaming
* New:	Added an alternate streaming strategy based on distance
* Fix:	Fixed a streaming bug where all the modules were loaded at start
* New:	Moved level streaming configuration to a separate category in the Dungeon actor.  It was previously in the Advanced section, not it is in the Level Streaming section

#### Misc
* New:	Redesigned the Dungeon Architect context menu for asset creation
* New:	Proto Tools has new materials with different color palettes
* Fix:	Fixed hanging issues when adjusting the Floor Planner configuration
* Fix:	Fixed Floor Planner outer wall alignment issue
* Fix:	Fixed a missing include that was causing compile issues on Linux
* New:	Disabled legacy plugin header format during builds in the Build.cs files
* New:	Added a blueprint function to check if the actor belongs to a certain dungeon actor
* New:	Authority Function check in the BP Function library for selector logics

Version 2.20.1 [04-Dec-2020]
--------------
* New: Added 4.26 engine support

Version 2.20.0 [10-Aug-2020]
--------------
* New: Grid Flow builder supports walls as edges.  This allows you to have thin walls around the edge of the tile.   (similar to the grid builder)
* New: SnapMap builder supports build retries.  When a dungeon fails to build with the specified grammar rules, it would try to rebuild (similar to the gridflow builder)
* New: SnapMap editor has various UX improvements.   There's a new Build button on the toolbar.  Removed unused buttons from the execution panel's toolbar
* New: SnapMap editor's result graph panel auto-zooms to fit the graph on screen after being built. This can be turned off from the settings button in the toolbar
* New: SnapMap Performance Analyser.  Run your SnapMap dungeon thousands of times in memory and analyse the performance in a graph (similar to the gridflow builder)
* New: Refactored the SnapMap builder code by moving the build logic out of USnapMapBuilder to a separate reusable class library
* New: Grid Flow editor's settings has a new field for specifying parameter overrides
* New: Blueprint library function to spawn dungeon owned actors. These spawned actors automatically get cleaned up when the dungeon is destroyed
* New: Grid Flow tilemap renderer shows a outline around the caves.   This also shows up in the minimap
* Fix: Dungeon Draw Editor Mode shows up correctly in 4.25
* Fix: Grid Flow editor's dungeon preview has proper sky lighting (no more black shadows)
* Fix: Grid Flow fence were sometimes not being spawned
* Fix: SnapMap builder no longer crashes when built with an empty flow graph
* Fix: Grid Flow ParameterOverride feature properly parses string arrays. (e.g. overring the list of paths to spawn an item works correctly)

Version 2.19.0 [6-May-2020]
--------------
* New: Engine 4.25 support added
* New: The Snap Connection editor uses Advanced Preview scene in the 3D viewport, so lighting works correctly and doesn't have dark shadows

Version 2.18.1 [13-Apr-2020]
--------------
* Fix:  Level streamer fires the InitialChunksLoaded event correctly only after all the nearby initial chunks are loaded (and not just the chunk that the player is standing on)

Version 2.18.0 [3-Apr-2020]
--------------
* New: Negation Volume support in SnapMap builder. This allows you to build your dungeons constrained inside a certain area, or avoid building it at certain areas
* New: Launch Pad Sample - Multiplayer enabled SnapMap dungeons
* New: Launch Pad Sample - Build SnapMap dungeons constrainted inside a certain area (using Negation Volumes with Reverse flag)
* New: Launch Pad Sample - Build SnapMap dungeons while avoiding certain area (using Negation Volumes)
* Fix: Fixed a crash issue when dungeons were built at runtime with landscape modifiers

Version 2.17.0 [15-Mar-2020]
--------------
* Critical Fix: Fixed a crash bug in stand alone builds when using the snap map builder and level streaming
* New: Grid Flow Query system to query the dungeon for gameplay and theme building.  [Video](https://youtu.be/YmOBN9-Wijo)
* New: Grid Flow Query samples in the Launch Pad
* New: Refactored some of the editor content path from PluginContent/* to PluginContent/Core/*. Added a CoreRedirect in Config/DefaultDungeonArchitect.ini to auto-fix reference issues in assets

Version 2.16.0 [8-Mar-2020]
--------------
* New: GridFlow Exec node parameters can be set at runtime from blueprint. [Video](https://youtu.be/uyDGfDHiLHo?t=5s)
* New: Performance Window in the Grid Flow Editor allows you to run thousands of tests on your graph and visualize the number or retries required to build the dungeons. [Video](https://youtu.be/uyDGfDHiLHo?t=709s)
* New: Support for static Start room position in the Grid Flow Builder. [Video](https://youtu.be/uyDGfDHiLHo?t=470s)
* New: Changed the GridFlow's room size from a number to Vector2.  Instead of square Rooms (NxN), you can now specify width and height (WxH). [Video](https://youtu.be/uyDGfDHiLHo?t=387s)
* New: Build GridFlow dungeons aligned with the Dungeon Actor at the center or at the tip of the tilemap. [Video](https://youtu.be/uyDGfDHiLHo?t=928s)
* New: Added a flag to enable tilemap cropping in the *Initialize Tilemap* node.  This will crop the tilemap around the dungeon layout, if enabled.  This was previously enabled, and is not disabled by default. [Video](https://youtu.be/uyDGfDHiLHo?t=670)
* New: SnapMap builder has a flag to disable module rotation. These are great for isometric / platformer levels
* New: Level Streamer has a bindable event to notify when all the initial chunks around the player has been streamed in.  These are great for builders like snap map where the player needs to be spawned after the initial modules are streamed in. Check the sample in launchpad: Samples > Snap Map Builder > Snap Runtime Demo
* New: Dungeon Builder object has a GetRandomStream blueprint function to access the random stream
* New: Launchpad sample entries have relavant documentation button
* New: Launchpad has new samples for GridFlow and Snap Map builder.  SnapMap samples no longer require you to download external assets. Clone it directly from inside the editor
* New: GridFlow assets have thumbnails
* New: Added a new entry to view the ChangeLog in the Launch Pad's Documentation section
* Fix: Fixed Snap Map level streaming issue while re-building the dungeon.  The previous dungeon modules properly get unloaded before building the next dungeon
* Fix: Fixed an Initialization warning while packaging the project
* Fix: Grid Flow Exec Graph context menu's "Abstract Graph" category has been renamed to "Layout Graph" for consistancy
* Fix: Exposed the Grid Flow Item info as ViewOnly in the details property window
* Fix: Disabled Collision on all dungeon volumes (except MarkerReplacementVolume, which is set to OverlapAll)

Version 2.15.0 [16-Feb-2020]
--------------
* New: Grid Flow Builder, a powerful new feature to control the flow and design of your dungeon layouts
* New: Browse samples and templates using the new LaunchPad window
* New: Added a new DA button in the level editor's toolbar to open the LaunchPad window
* New: Updated Level Streaming framework to support multi-player
* New: Snap Flow Editor's backtracking in the Debug View shows a blue outline around the active module
* New: The plugin contains all the samples content that can be accessed from the launchpad, and no longer requires a separate download from the quick start guide
* New: Removed all starter content dependencies from the samples
* New: Updated DA Logo in the editor mode's window
* New: Removed the HelpSystem module and merged it with the Editor Module
* Fix: Fixed compile errors in Visual Studio 2019

Version 2.14.2 [5-Jan-2020]
--------------
* Fix: Fixed streaming issues in the Snap Map builder when the Flow Editor's Debug view is opened
* New: Flow Editor's Debug mode's backtrack steps hilights the current module node in blue color

Version 2.14.1 [4-Jan-2020]
--------------
* Fix: Fixed an issue which was causing standalone build creation to fail
* Fix: Collision profile set in the theme editor's mesh node properly propages to the instanced meshe dungeons (HISM)

Version 2.14.0 [10-Dec-2019]
--------------
* New: Added 4.24 Engine support
* Fix: Fixed a stability issue in the Dungeon Flow editor's visualize tab
* Fix: Snap Map builder now takes into account the connection constraints (magnet, male, female)

Version 2.13.2 [5-Sep-2019]
--------------
* New: Added 4.23 Engine support
* New: Save / Load custom state while the dungeon is being built.  This allows you to track the no. of objects spawned (by setting or incrementing a variable) and later checking against it from selector / transform / spawn blueprints
* New: Added HTML5 support

Version 2.13.1 [22-May-2019]
--------------
* Fix: Fixed unloading issues with the snap map builder's module levels.  This was causing issues while saving maps and taking up memory
* Fix: Actor replication in snap map modules were not working correctly

Version 2.13.0 [2-May-2019]
--------------
* New: Major upgrade to the Dungeon Flow editor with the new Visualize and Debug tabs on the top right. More info [here](https://www.youtube.com/watch?v=BfiOLFo5uDU)
* Fix: Pasted markers in the theme editors were not being deleted in some cases

Version 2.12.0 [4-Apr-2019]
--------------
* New: Added 4.22 Engine support
* New: Added weights to theme override volumes so if they overlap, the volume with the larger weight gets its theme applied
* New: Theme editor has an option to force rebuild the preview viewport's dungeon layout, which will be a little slow but more accurate, in case of custom marker emitter blueprint usage
* New: Removed Apex Destructible plugin dependency from Dungeon Architect and the custom Destructible mesh node support. However, destructible meshes are still supported.  (Drag drop your destructible meshes on the theme editor)
* Fix: Paint mode now properly shows the colored layout, which was not being shown in some cases
* Fix: CityBuilder adds large city blocks (greater than 3x3) correctly
* Fix: Marker emitters on custom placed platforms (using platform volumes or painted cells) were having their Z registered incorrectly making objects float on air
* Fix: Improved the debug draw's performance in the theme editor's viewport window


Version 2.11.2 [15-Feb-2019]
--------------
* Fix: Fixed a packaging issue on older engine versions by performing a check while shutting down the plugin

Version 2.11.1 [13-Feb-2019]
--------------
* Fix: Fixed an issue where SnapMap builder was not connecting properly to duplicated connection actors
* Fix: Added Category to UFUNCTIONs to avoid compile errors while packaging
* Fix: Disabled finding of default minimap material to avoid warning popup dialogs

Version 2.11.0 [6-Feb-2019]
--------------
* New: Dungeon Flow Editor - Design and control the flow of your dungeons using this powerful new feature using Graph Grammars
* New: Snap Map Builder: A new builder that allows you to stitch pre-made rooms together using the dungeon flow system. This builder has now been moved out of work in progress state
* New: Mini Map framework - Generate procedural mini-maps for your dungeons and decorate them with your materials. It supports fog of war, static and dynamic icons and other features
* New: Level Streaming Framework - A reusable node based level streaming framework that loads only the nearby accessible rooms on demand, keeping the framerate high
* New: Level Streaming Navigation - Optimized level navigation system that properly updates as new new chunks are streamed in
* New: Proto Tools - New Prototyping art assets to quickly create dungeon pieces and rooms for your SnapMap builder
* New: Modes Category - The modes window now has placement categories for Dungeon Architect and ProtoTools to easily find and drop related actors on to the scene
* New: Added a sample game to demonstrate the new Snap Map dungeon builder
* New: Added blueprint access to cluster themes
* New: Theme editor supports drag-drop of any type of asset from the content browser. If it doesn't understand an asset (e.g. Paper2d, Prefabricator), it would create an actor node and ask the engine to initialize the template. This removes the previous limitaion of supporting third-party plugins
* New: Theme editor shows better thumbnail for the actor node by showing the thumbs of the asset it uses. If it cannot find one, it falls back to the class type icon like before
* Fix: Fixed a null pointer issue with the marker emitter node in some cases of bulk deletion
* Fix: Fixed all level streaming stability issues
* Fix: The snap map builder generates the dungeon from the Dungeon Actor's location instead of (0,0,0)
* EXPERIMENTAL: Grid Builder Thick Wall/Door Support - Redesigned the grid builder layout strategy to fully support thick walls and doors (i.e. walls/doors taking up one full tile, instead of an edge)


Version 2.10.0[17-Nov-2018]
--------------
* New: Added Engine 4.21 support
* New: Added a search field and scroll bar in the theme editor's Markers window
* New: Added a new 'Dungeon Architect' category in the Level editor's Modes panel. This lets you easily place dungeon related actors in the level
* New: Added an experimental cell distribution method in the Grid Builder. It is fast and lets you create large dungeons by specifying the dungeon width and height. More info here: https://coderespawn.page.link/jTpt
* New: The Mission graph editor's (Dungeon Flow) RHS graph grammar rule take in weights and are randomly selected based on the weights during substitution
* New: Added blueprint access to the cluster theme list in the dungeon actor
* Fix: Fixed an optimization issue with the grid builder
* Fix: Fixed a Content Browser spawn bug in the theme editor


Version 2.9.3 [17-Sep-2018]
-------------
* Fix: Fixed a crash issue on stand alone builds due to editor only objects in snap connection actor
* New: Added IsNearMarker function back in.   This function is useful for querying nearby markers in the scene from your transform / selector logic BP

Version 2.9.2 [31-Jul-2018]
-------------
* Fix: Fixed compile issue with the snap map connection actor

Version 2.9.1 [26-Jul-2018]
-------------
* Fix: Fixed a crash issue in standalone builds launched from the editor
* New: Level streaming supports unloading of hidden maps from memory

Version 2.9.0 [19-Jul-2018]
-------------
* New: Added 4.20 Engine compatibility
* New: Added level streaming support on the Snap Map builder
* New: Early implementation of the Dungeon Flow system using graph grammar (mission editor)
* Fix: Improved the stability of Snap Map builder

Version 2.8.3 [16-Mar-2018]
-------------
* New: Added 4.19 Engine compatibility
* Fix: Custom markers emitted from the theme file are visible in the event lister's OnMarkerEmitted event

Version 2.8.2 [14-Nov-2017]
-------------
* Fix: Fixed an error while packaging the project

Version 2.8.1 [6-Nov-2017]
-------------
* New: Restored the old snap builder and moved the new snap builder that was introduced in 2.8.0 to a separate builder named "SnapMap"

Version 2.8.0 [3-Nov-2017]
-------------
* New: Added 4.18 engine compatiblity
* New: Redesigned the snap builder.  The snap modules can now be designed in individual map files
* New: Added an editor for the Snap connections which lets you define the door and the wall meshs
* New: Added a generic graph based Level streaming framework which the other builders can use
* New: The snap builder now supports level streaming
* New: Updated the C++ code to use IWUY headers to make the compilation in dev mode faster.   Since this feature is available in the engine since 4.15, the minimum compability has been changed from 4.12 to 4.15
* New: Refactored and cleaned up the landscape rasterizer code
* New: Since the APEX Destruction code in unreal engine has been moved to a plugin system, DA now references this plugin as a dependency
* Fix: Static Mesh mobility is correctly set after spawning from a mesh theme node
* Fix: Landscape rasterizer now produces correct results when the the landscape actor is not in origin
* Fix: Duplicated dungeon actors no longer destroy the meshes from the other dungeon actor on rebuilds
* Fix: Fix Snap builder door connection issues


Version 2.7.0 [31-Jul-2017]
-------------
* New: Marker replacement volumes now run after all the Marker Emitter blueprints have emitted their markers
* New: Improved the stair connection model
* New: Refactored the SceneProvider C++ API to take in a context object
* New: Exposed the snap confiugration to blueprints
* New: Exposed the grid tool paint data to blueprints
* New: Added wall separators to floorplan builder
* Fix: Fixed a bug in the grid builder that was causing doors to not create properly in some cases
* Fix: Fixed a crash issue with the Actor theme node
* Fix: Fixed a bug with pooling where actors from another dungeon where deleted

Version 2.6.3 [5-Jun-2017]
-------------
* Fix: Fixed a critical bug with theme files that was crashing standalone builds
* New: IsNearMarker grid query function also retuns the number of nearby markers found nearby

Version 2.6.2 [25-May-2017]
-------------
* New: Added 4.16 engine support
* New: The paint tool can now also paint rooms instead of just corridors
* Fix: Fixed a bug with the paint tool now working with multiple dungeons in the same map

Version 2.6.1 [7-May-2017]
-------------
* New: Added a new help system to launch the documentation links from within the editor.  This can be invoked from the Dungeon Actor's details panel or from the theme editor's toolbar
* New: Added height variations to the custom grid builder
* Fix: Fixed static lightmap baking issues with instanced mesh dungeons
* Fix: Fixed a bug with PostDungeonBuild Listener Event which was getting fired before all the dungeon actors were spawned
* Fix: Fixed Spatial Constraints not working on standalone build
* Fix: Fixed a crash issue if Landscape modifier is enabled and the dungeon is built at runtime
* Fix: Fixed a crash issue with the IsNearMarker node if the builder pin was not plugged in

Version 2.6.0 [10-Apr-2017]
-------------
* New: Landscapes can now be automatically modified (height and paint data) when the dungeon is built.  Since the engine supports only landscape modification from within the editor, this works only at design time and not runtime
* New: Added a content browser to the theme editor window
* New: Theme asset files now show a thumbnail, captured from the preview viewport.
* New: Added drag drop support on the theme editor.  You can now drag and drop meshes, blueprints, particle systems etc to the theme editor and a node will automatically be created
* New: Added more parameters to the Grid Transform Logic, similar to the parameters in the Grid Selector Logic
* New: Added spatial constraint on the simple city builder
* New: Selectors can now query nearby markers. These are great for not spawning blocking items near doors and stairways
* New: Added a custom grid builder that makes it easier to create your own dungeon layouts using high level blueprint functions
* New: Fixed Snap door state issues on level reload
* New: Increased the default value of grid config's Door Proximity Steps from 2 to 6 for better quality door removal by default on dungeons
* New: Added a new snap demo level in the quick start guide

Version 2.5.0 [16-Feb-2017]
-------------
* New: Added support for 4.15
* New: Updated the road network of the City builder to randomly remove edges and create a more natural looking layout
* New: Added a new function in the Grid Query System to find adjacent cells from an edge marker (like wall / fence). New example added under DA_Misc_Examples/Maps/WallQueryDemo
* New: Added addtional parameters (Query System and World Transform) to the Grid Selector Logic for more control
* New: Increased the default stair connection tollerance from 3 to 6 for better results with the default configuration
* New: Rewritten the snap builder to use blueprints, instead of levels for modules.  This greatly improves the performance and stability.  Still has issues with door states and remains a work in progress
* New: Added a snap demo example with 20 modules of various sizes
* Fix: Houses in the City Builder now face the road side.   House cells not accessible through roads are promoted to Parks
* Fix: Fixed initialization issues with the actor node in the theme editor

Version 2.4.0 [19-Dec-2016]
-------------
* New: Clustered theming allows different themes to be automatically applied to different parts of the dungeon.  This helps in adding variation to the level
* New: Event listeners can now modify the marker positions before they are sent to the theming engine, giving more low level control
* New: Added an experimental city builder to build simple city layouts
* New: Added variations to the city builder by randomly merging nearby city blocks
* New: City blocks of custom sizes are supported (e.g. 1x2, 4x5 etc).  This helps in placing larger structures (e.g. parks, parking lots, larger buildings etc)
* New: Added a helper function in the Isaac builder to clear the styling information from specified isaac room (useful for creating large spawn / boss rooms)
* New: Added new misc examples in the quick start guide
* Fix: Fixed scaling issue with actor nodes
* Fix: Fixed physics errors with certain actors while rebuilding the dungeon. Actors spawned by the actor node are not reused on a dungeon rebuild. Instead, they are destroyed and recreated to avoid physics errors
* Fix: Dungeons spawn in the correct level when used with world composition, instead of always spawning in the persistant level
* Fix: Fixed lightmass warnings on light actors while building lighting

Version 2.3.1 [27-Nov-2016]
-------------
* Fix: Fixed various bugs with Actors not spawning correctly from the Actor Node in some cases
* Fix: Fixed a crash issue while copying Actor nodes in the theme editor

Version 2.3.0 [18-Nov-2016]
-------------
* Fix: Fixed a critical bug with runtime dungeons not spawning meshes properly
* New: Actor Nodes are now customizable.   Blueprints or actors spawned by the Actor node can have any of their properties customized from the theme editor
* New: Added Spawn Logic Blueprints, which are attached to the visual nodes and get executed when the actor is spawned on the scene, giving more control on the customization of the spawned actor
* New: Display a "Work In Progress" warning message in the Dungeon actor's details window if an experimental builder is selected
* New: Added Documentation entries for Dungeon Architect in the editor's Help menu.   The quick start guide, user guide and video tutorials can be launched from there

Version 2.2.1 [16-Nov-2016]
-------------
* New: Added 4.14 engine compatibility
* New: Added a RemoveDuplicateDoors flag in the snap config to remove/retain the adjacent door between two snap modules
* New: Use a cleaner engine API to copy the properties of actor templates in the theme graph (for mesh and light nodes)
* Fix: Fixed navigation issues with the snap builder

Version 2.2.0 [10-Oct-2016]
-------------
* New: Major improvements on the Snap Builder.  It uses world composition to stream modules in
* New: Created a Query interface for advanced gameplay logic
* New: Added new set of examples to demostrate the Query interface (for grid and isaac builder).  Refer the examples in the quick start guide (DA_Query_Examples)
* New: Added a Marker Replacement volume to detailed fine tuning of the level. You can use this to manually add / remove doors, walls etc after the dungeon is built
* New: Added foliage support to the Snap Builder
* New: Created a new type of builder named SimpleCity (Experimental)
* Fix: Various bug fixes in the snap builder

Version 2.1.11-beta.2 [30-Sep-2016]
-------------
* New: On dungeon destroy / rebuild, unused module instances are cleaned up properly
* Fix: Fixed a crash bug by streaming in the modules in the game thread

Version 2.1.11-beta.1 [26-Sep-2016]
-------------
* New: Snap builder now uses world composition to build the scene.   This retains all the other engine features in the modules like lightmaps, foliage, level blueprint scripts etc

Version 2.1.10 [18-Sep-2016]
-------------
* New: Added support for branch end modules in the snap builder
* New: Added a flag in the Snap Module Info actor to ignore module count when inserted into the scene.  This is useful if you do not want certain types of modules (e.g. small corridors) to contribute to the overall module count
* New: The Snap door caption is now hidden while playing
* Fix: Fixed a bug with duplicate WallHalf and WallHalfSeparator meshes in the Grid Builder
* Fix: Fixed a bug where overlapping fence and walls were created with some elevated rooms in the Grid Builder
* Fix: Optimized the performance of the Snap Builder by fixing a caching issue
* Fix: Fixed a bug where the end modules were also created in the end of branches in the snap builder

Version 2.1.9 [16-Sep-2016]
-------------
* New: Added a new Destructible Mesh node in the theme graph
* New: Major improvements to the snap builder algorithm
* New: Updated the quickstart guide with a new DeathMatch multiplayer demo
* New: Snap builder dungeons are created relative to the dungeon actor's transformation instead of starting from origin
* New: The Door editor hides the Static Mesh field when blueprint type is selected from the drop down list
* New: Added a sprite to the SnapModuleInfo actor for better usability
* Fix: Fixed a stair issue where stairs attached to redundant doors were not removed along with the doors

Version 2.1.8 [8-Sep-2016]
-------------
* New: Added improvements to the snap builder algorithm with a better search / growth algorithm
* New: Added module info actor to specify metadata about the actors
* New: Doors can now attach only to certain types of modules, as specified in the metadata

Version 2.1.7 [5-Sep-2016]
-------------
* New: Improved the snap builder algorithm by adding more exhaustive search while building
* New: Other small improvements to the grid builder and snap builder algorithms
* Fix: Fixed a bug in the grid builder which was sometimes causing unreachable corridors

Version 2.1.6 [4-Sep-2016]
-------------
* New: Added 4.13 engine compatability
* New: Added a preview settings tab in the theme editor to support the engine's new advanced viewport preview feature introduced in 4.13

Version 2.1.5 [1-Sep-2016]
-------------
* Fix: Fixed adjacent door issues with the Grid builder.   Redundant doors are no longer created and it can be controlled through a parameter

Version 2.1.4 [31-Aug-2016]
-------------
* Fix: Fixed a crash issue in non-editor build due to a bug in the SnapDoor actor

Version 2.1.3 [11-Aug-2016]
-------------
* New: Added a Start / End Module in the snap builder for spawn and final boss fight rooms
* New: Implemented the MaxDepth parameter in the Snap builder. This lets you control how deep a branch can go from the starting point
* New: Snap Doors actors show the name of the door asset on top of them to differentiate between different door types during module design
* New: Added tooltips to the snap builder configuration


Version 2.1.2 [9-Aug-2016]
-------------
* New: Theme editor now shows the dungeon properties by default if nothing is selected
* New: The default marker for the selected builder are automatically created in the theme editor when the builder type is changed from the properties window
* New: The snap builder connects modules together based on matching door assets.
* New: Floor plan buildings can be created anywhere on the map, based on the dungeon actor's transform (arbitrary volumes are not working yet)
* New: Ported all the quick start samples to version 2.x
* New: Created a new Quick Start guide in the new documentation platform
* New: Removed the save dialog location prompt while upgrading a version 1 theme file. The upgraded file is created in the same directory with a "_v2" prefix
* Fix: Fixed runtime generation issues with the snap builder dungeon due to mesh mobility
* Fix: Fixed breaking changes in C++ under OSX
* Fix: Fixed a bug in the cell comparison function of the grid builder
* Fix: Fixed all header compile errors in C++ if compiled without Unity build flag set to false in the build configuration

Version 2.1.1-beta [27-Jul-2016]
------------------
* New: Added 4.12 engine compatability

Version 2.1.0-beta [16-Jul-2016]
------------------
* New: Implemented the Snap Builder algorithm.

Version 2.0.0-beta [14-May-2016]
------------------
* New: Initial version of 2.0

Version 1.5.0 [7-Feb-2016]
-------------
* New: Major improvements in the stair connection system.  It has been re-written to be more robust
* New: Added selector affinity override flag, so that the selector logic can optinally respect the affinity varaible
* New: Improved the Graph node transform property editor for multiple selections.  Moved the node offset to a separate category in the details view for better usability
* New: The theme file can now be referenced from blueprints.  This lets you switch themes on a level during runtime with blueprints. New demo added showcasing this
* Fix: Fixed a crash issue when some actors (e.g. characters) were spawned in map using the actor node
* Fix: Fixed various bugs with doors, platform volumes and stairs

Version 1.4.0 [3-Dec-2015]
-------------
* New: Major performance improvements in the theme graph editor.  The preview scene is populated over multiple frames to avoid stalling the UI thread and making the UI more responsive
* New: Added an experimental Cave Builder.  Added a cave builder sample to the quick start guide
* New: Added 2D version of the Cave Builder.  Added a 2D cave sample to the quick start guide

Version 1.3.0 [14-Nov-2015]
-------------
* New: Added a Mirror Volume for creating symmetrical dungeon layouts (e.g. for strategy /.moba games)
* New: Mesh nodes have a "Advanced Options" section for more control (e.g. for setting physics, shadowing etc)
* New: Internal refactoring to make DA more modular and extensible
* New: Created extension points to let external modules plug in new type of nodes into the theme editor (e.g. paper2d nodes)
* New: Created extension points to let an external scene provider render any type of geometry in the theme editor's viewport without knowing the implementation details (e.g. sprites)
* New: Added new demos
* Fix: Fixed a bug where static actors were not having their transforms updated when the dungeon was rebuilt at runtime
* Fix: Dynamically spawned light actors no longer give mobility warning message in the output log (fix suggested by sch5, Thanks!)
* Fix: Lights were not spawned in dungeons when it was rebuilt at runtime (fix suggested by sch5, Thanks!)

Version 1.2.1 [11-Nov-2015]
-------------
* New: Added marketplace URL in the uplugin manifest file
* New: Added the dungeon theme entry to the "Basic Assets" section of the content browser context menu
* Fix: Fixed the plugin styling code not finding the icons on the Editor Mode tab

Version 1.2.0 [5-Nov-2015]
-------------
* New: Improved randomness in the builder algorithm by removing repeated reinitializations on the random stream
* New: Added random stream parameter to selector logic and transform logic
* New: Added helper functions get query the room spatial dimentions
* New: Added variable lane width customization in the dungeon builder config
* Fix: Disabled threading on the builder as the layout generator is not a bottle neck but was causing issues running blueprints in a different thread. The layout is built on the main game thread and the meshes are spawned over multiple frames
* Fix: Fixed a dungeon building bug in the theme browser's preview port where the dungeon layout was not built accurately as per the config

Version 1.1.5 [27-Oct-2015]
-------------
* New: Added a docs folder with user guide, video tutorials and quick start sample pages
* Fix: Fixed a dungeon selection bug in the editor paint mode.  Users can now switch to a different dungeon to paint on by clicking on the dungeon actor in the world outliner

Version 1.1.4 [22-Oct-2015]
-------------
* New: Added the ability to preview custom markers within the theme editor (Theme Editor > Preview 3D Viewport > Properties > Dungeon > Marker Emitters)
* New: Added helper functions to deal with cell bounds more easily from within blueprints
* New: Added the ability to preview actors in the 3D preview viewport of the theme editor
* New: Added copyright notice to source files
* Fix: Fixed a build issues

Version 1.1.3 [21-Oct-2015]
-------------
* New: Added backward compatibility to theme files when instanced mesh scene provider is used
* New: Added a custom Marker Emission framework.  Users can now emit their own markers anywhere in the scene using blueprints
* New: Added NotBlueprintable to Dungeon actor's UCLASS macro so that the dungeon actor cannot be converted into a blueprint
* Fix: Fixed a crash issue in the dungeon actor if no builder is assigned.
* Fix: Removed the base selector and transform rule classes from the drop down list in editor as they are meant to be implemented, rather than used

Version 1.1.2 [21-Oct-2015]
-------------
* Fix: Fixed issues with game code not building due to editor specific code. Added WITH_EDITOR macro
* Fix: Fixed breaking changes with XCode clang compiler to make it work with OSX

Version 1.1.1 [18-Oct-2015]
-------------
* Fix: Fixed instanced mesh builder crash issue
* Fix: Fixed a crash issue due to particle systems not setting their templates in the main game thread

Version 1.1.0 [12-Sep-2015]
-------------
* New: Added billboard component to the dungeon actor
* New: Added tooltips to the various fields in the dungeon configuration structure
* New: Added tooltips to the theme graph node property fields
* New: Added undo support in the dungeon paint mode
* New: Refactored the dungeon marker emission API so users can design their own dungeon algorithms within blueprints or C++
* New: Added an experimental organic builder that works with landscapes.   The layout is carved into the landscape (work in progress)
* New: Added a new button in the dungeon actor to modify the dungeon builder's properties (e.g. a user defined BP based dungeon builder can expose customizable parameters, and these can be edited from the dungeon actor)
* New: Spawned dungeon items are placed a dungeon specific folder to avoid cluttering up the scene outliner
* New: Dungeon model is now accessible from blueprints.  This lets you loop through the cells and doors from blueprint
* Fix: Instanced static mesh generator now properly supports material override
* Fix: Fixed a crash issue with entering dungeon paint mode when the scene has no dungeon actors
* Fix: In Paint editor mode, the cursor is not shown when the camera is moved around
* Fix: Fixed an infinite loop error when a cycle is formed in the graph.   Cycles are now disallowed by the theme graph editor

Version 1.0.0 [11-Sep-2015]
-------------
* New: Initial version

Version 0.2.9 [08-Sep-2015]
-----------------
* Fix: Fixed breaking changes on UE4 4.9 for Windows and Mac
* Fix: Disabled the Selector instance spawner pool database.   The template defined by the user is used directly.  (See known issues)

Version 0.2.8 [31-Aug-2015]
-----------------
* New: Added dungeon repositioning based on the dungeon actor's offset
* New: Applying a negation volume pass early on to remove room connections from rooms that are within the volume
* New: Added reverser flag to negate the effect of volumes.  This is useful to negate the negation logic (i.e. remove everything inside the volume, or everything outside the volume).  Also works with theme override
* New: Since volumes cannot be spawned from the editor, added a feature to use the transform of the volume as bounds
* New: Added a base dungeon volume for all the other volume actors. Fixed dungeon height offsets relative to the actor's transform
* New: Moved grid cell lookup from the dungeon builder to the dungeon model
* Fix: Removed redundant assignment in the scene provider's constructor.

Version 0.2.2 [06-Aug-2015]
-----------------
* Fix: Changed the unique identifier used for tagging the dungeon objects. Swithced to a GUID instead of the engine assigned object id since it changes when the map is reloaded

Version 0.2.1 [05-Aug-2015]
-----------------
* Fix: Fixed a bug where empty static mesh actors were being created with null static mesh references

Version 0.2.0 [05-Aug-2015]
-----------------
* Fix: Fixed a bug where components were not properly registered in the blueprint based instanced mesh actor

Version 0.1.0 [04-Aug-2015]
------------
* New: Added support for Unreal Engine 4.8
* New: Paint tool for manually authoring the dungeons
* New: Support Async dungeon building and multi-threading framework
* New: Negation volumes for disallowing dungeon generation in certain areas
* New: Actor theme node to spawn interactive blueprints form the theme engine
* New: Implemented Instanced Mesh support
* Fix: Fixed light theme node rendering issues
* Fix: Fixed stair generation near doors
* Fix: Various bug fixes on Engine 4.7
