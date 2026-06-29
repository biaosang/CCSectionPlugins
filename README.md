# qSectionProfile for CloudCompare

This repository contains a CloudCompare standard plugin that extracts section profile samples from one selected polyline and one selected point cloud.

## Features

- Select one `ccPolyline` and one `ccPointCloud` in CloudCompare.
- Extract points within a configurable half width around the polyline.
- Preview a chainage/elevation profile.
- Save the profile as PNG or JPEG.
- Export the extracted section samples to CSV with `chainage,elevation,offset,x,y,z,segment_index`.

## Build on macOS with CloudCompare 2.14 beta source

1. Copy or symlink `qSectionProfile` into CloudCompare's `plugins/private` directory.
2. Add the plugin folder from CloudCompare's plugin CMake list if your source tree does not auto-discover private plugins.
3. Configure CloudCompare with CMake and enable `PLUGIN_STANDARD_QSECTION_PROFILE`.
4. Build CloudCompare normally.

The plugin is intentionally implemented with Qt Widgets painting instead of QtCharts, so it does not require an extra Qt module beyond the usual CloudCompare GUI dependencies.

## Build with GitHub Actions

This repository includes `.github/workflows/build-macos.yml`.

1. Push this repository to GitHub.
2. Open the repository's **Actions** tab.
3. Run **Build macOS CloudCompare Plugin**.
4. Set `cloudcompare_ref` to the CloudCompare branch or tag you want. Use `master` for current 2.14+ development sources, or a specific 2.14 beta tag/commit when you have one.
5. Download the `qSectionProfile-macos` artifact after the job completes.

The workflow uses CloudCompare's own macOS conda dependency file, copies `qSectionProfile` into `CloudCompare/plugins/private/qSectionProfile`, enables `PLUGIN_STANDARD_QSECTION_PROFILE`, and builds the plugin target.

## Current extraction model

Each cloud point is projected to the nearest polyline segment in the plane perpendicular to the selected elevation axis. Points whose planar distance to that segment is less than or equal to the selected half width are kept. The exported `chainage` is the planar distance along the polyline to the projected point, and `offset` is signed from the segment direction when possible.
