# OccQtCore 開発環境メモ

## 概要

`OccQtCore` は、Qt + OpenCascade を使った STEP ビューア / 形状解析基盤プロジェクトです。

今後、穴認識、ポケット認識、パス表示、形状解析などの小規模アプリで共通利用できる基盤として整理していく予定です。

---

## プロジェクト構成

| 項目 | 内容 |
|---|---|
| プロジェクト名 | `OccQtCore` |
| プロジェクトパス | `C:/work/OccQtCore` |
| モデル置き場 | `C:/work/OccQtCore/model` |
| ビルド方式 | CMake |
| C++標準 | C++17 |
| 主なIDE | Qt Creator |
| 補助IDE | Visual Studio 2022 |

---

## Qt 環境

| 項目 | 内容 |
|---|---|
| Qt Version | `6.9.1` |
| Qt Path | `C:/Qt/6.9.1/msvc2022_64` |
| Qt Lib Path | `C:/Qt/6.9.1/msvc2022_64/lib` |
| qmake Version | `QMake version 3.1` |
| Build / Kit | MSVC2022 64bit |

確認コマンド:

```bat
C:\Qt\6.9.1\msvc2022_64\bin>qmake --version
QMake version 3.1
Using Qt version 6.9.1 in C:/Qt/6.9.1/msvc2022_64/lib
```

---

## OpenCascade / OCCT 環境

| 項目 | 内容 |
|---|---|
| OCCT Version | `7.9.1` |
| Compiler | `vc14` |
| Platform | `win64/vc14` |
| Shared Libs | ON |
| Debug Build | FALSE |

`OpenCASCADEConfig.cmake` 上のバージョン定義:

```cmake
set (OpenCASCADE_MAJOR_VERSION       "7")
set (OpenCASCADE_MINOR_VERSION       "9")
set (OpenCASCADE_MAINTENANCE_VERSION "1")
set (OpenCASCADE_DEVELOPMENT_VERSION "")
```

主なパス:

| 項目 | パス |
|---|---|
| Binary Dir | `${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/bin` |
| Library Dir | `${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/lib` |
| Include Dir | `${OpenCASCADE_INSTALL_PREFIX}/inc` |
| Resource Dir | `${OpenCASCADE_INSTALL_PREFIX}/src` |

OpenCascade の主な設定:

```cmake
set (OpenCASCADE_COMPILER          "vc14")
set (OpenCASCADE_BUILD_WITH_DEBUG  FALSE)
set (OpenCASCADE_BUILD_SHARED_LIBS ON)
```

有効なサードパーティ設定:

```cmake
set (OpenCASCADE_WITH_TCL       ON)
set (OpenCASCADE_WITH_FREETYPE  TRUE)
set (OpenCASCADE_WITH_FREEIMAGE TRUE)
set (OpenCASCADE_WITH_TBB       TRUE)
set (OpenCASCADE_WITH_VTK       TRUE)
set (OpenCASCADE_WITH_FFMPEG    TRUE)
set (OpenCASCADE_WITH_GLES2     TRUE)
set (OpenCASCADE_WITH_D3D       TRUE)
```

---

## 使用中の主な OpenCascade モジュール

`OpenCASCADEConfig.cmake` 上では以下のモジュールが利用可能です。

```txt
FoundationClasses
ModelingData
ModelingAlgorithms
Visualization
ApplicationFramework
DataExchange
DETools
Draw
```

現在のプロジェクトでは主に以下を使用します。

| 用途 | 代表ライブラリ |
|---|---|
| 基本機能 | `TKernel`, `TKMath` |
| トポロジー / BRep | `TKBRep`, `TKTopAlgo`, `TKG3d`, `TKG2d`, `TKGeomBase` |
| 表示 | `TKService`, `TKV3d`, `TKOpenGl` |
| STEP読み込み | `TKDESTEP`, `TKXSBase`, `TKDE` |
| 形状生成 / Boolean | `TKPrim`, `TKBool`, `TKBO` |

