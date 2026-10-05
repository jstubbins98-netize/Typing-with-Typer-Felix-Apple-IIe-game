# Third-party components

The packaged `FELIX.SYSTEM` is the unmodified `loader.system` supplied by
the installed cc65 toolchain. The executable also links the cc65 runtime.

cc65 project: https://github.com/cc65/cc65

cc65 uses the following permissive license (some separately identified files
in its distribution have their own notices):

> This software is provided 'as-is', without any expressed or implied
> warranty. In no event will the authors be held liable for any damages
> arising from the use of this software.
>
> Permission is granted to anyone to use this software for any purpose,
> including commercial applications, and to alter it and redistribute it
> freely, subject to the following restrictions:
>
> 1. The origin of this software must not be misrepresented; you must not
>    claim that you wrote the original software. If you use this software
>    in a product, an acknowledgment in the product documentation would be
>    appreciated but is not required.
> 2. Altered source versions must be plainly marked as such, and must not
>    be misrepresented as being the original software.
> 3. This notice may not be removed or altered from any source distribution.

## ProDOS 2.4.3

The bootable game disk includes the unmodified boot blocks and PRODOS file
from the official ProDOS 2.4.3 release, by Apple Computer and John Brooks
and Friends. These components retain their original copyright notices;
they are not original game code or covered by the cc65 license above.

Release information: https://prodos8.com/releases/prodos-243

Official distribution: https://github.com/ProDOS-8/ProDOS8-Releases

See `vendor/README.md` for the exact source and checksum. This is a custom
game disk, not an unmodified official ProDOS distribution disk.

No Apple ROM images are included.