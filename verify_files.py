#!/usr/bin/env python3
import os

files = [
    "CMakeLists.txt",
    "sdkconfig.defaults",
    "partitions.csv",
    ".gitignore",
    "README.md",
    "main/CMakeLists.txt",
    "main/Kconfig.projbuild",
    "main/project_config.h",
    "main/app_main.c",
    "main/app_driver.c",
    "main/app_driver.h",
    "main/sensor_task.c",
    "main/sensor_task.h",
    "main/cloud_task.c",
    "main/cloud_task.h",
    "main/display_task.c",
    "main/display_task.h",
    "main/alert_task.c",
    "main/alert_task.h",
    "main/ota_task.c",
    "main/ota_task.h",
    "components/dht11/CMakeLists.txt",
    "components/dht11/dht11.h",
    "components/dht11/dht11.c",
    "components/ssd1306/CMakeLists.txt",
    "components/ssd1306/ssd1306.h",
    "components/ssd1306/ssd1306.c",
    "components/ssd1306/font8x8_basic.h",
    "docs/CIRCUIT_DIAGRAM.md",
]

print("Checking for required files...")
missing = []
for f in files:
    if os.path.exists(f):
        print(f"✓ {f}")
    else:
        print(f"✗ {f} MISSING!")
        missing.append(f)

print(f"\n{len(files) - len(missing)}/{len(files)} files present")
if missing:
    print(f"\nMissing {len(missing)} files:")
    for f in missing:
        print(f"  - {f}")
else:
    print("\n✅ All files present! Ready to build.")