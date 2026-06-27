# Dungeon Architect Plugin for Unreal Engine
Copyright © 2015-2025, Code Respawn Pvt Ltd. All Rights Reserved.

## License Grant
This is a single-seat license for the Dungeon Architect Plugin for Unreal Engine. This plugin is subject to and
governed by the Unreal Engine EULA (End User License Agreement).

## Core Restrictions

### Editor-Runtime Separation
The code and functionality from the Editor module MUST NOT be moved, copied, or reimplemented in the Runtime module. 
This includes but is not limited to:
- Moving editor-specific code to runtime environments
- Creating standalone applications using editor module components
- Redistributing editor functionality in runtime builds

### Code Redistribution
- Source code must not be redistributed to anyone who does not possess a valid license
- Teams may set up shared repositories, provided that every team member with access has purchased their own license
- Each individual accessing, using, or viewing the source code must have their own valid license

## Usage Rights

### Code Modification
- You may modify the source code for your own projects
- Modified code can be shared with team members who possess valid licenses
- All modifications must comply with the Core Restrictions above

### Demo and Sample Content
- All demo and sample content included with the plugin can be freely used and modified in any project
- Modified demo content is bound by the same license terms regarding redistribution

## Updates and Support
- All future updates to the plugin are included with the purchase
- Technical support is provided via the official Discord channel
- Response times for support are not guaranteed

## Third-Party Acknowledgments

### Delaunay Triangulation Implementation
- Author: Paul Bourke
- Source: http://paulbourke.net/papers/triangulate/
- Reference: http://paulbourke.net/papers/triangulate/triangulate.c
- License: Public Domain
- Usage Location: Source/DungeonArchitectRuntime/Private/Core/Utils/Triangulator/Impl/DelauneyTriangleGenerator.cpp

### Fast Gaussian Blur Implementation
- Source: http://blog.ivank.net/fastest-gaussian-blur.html
- License: MIT (https://opensource.org/licenses/MIT)
- Usage Location: Source/DungeonArchitectRuntime/Public/Core/Utils/MathUtils.h (BlurUtils)

## Contact Information
For licensing inquiries, please contact:
Code Respawn Pvt Ltd
support@coderespawn.com