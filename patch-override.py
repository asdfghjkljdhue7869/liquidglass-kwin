import sys

base = sys.argv[1]
c = base + "/liquidglass.cpp"
s = open(c).read()
if "shader override" in s:
    print("already patched")
    sys.exit(0)
if "sharpLocation" not in s:
    print("run the 4e patch first (sharpLocation missing)")
    sys.exit(1)


def patch(old, new):
    global s
    if s.count(old) != 1:
        print("ANCHOR NOT FOUND OR NOT UNIQUE: " + old[:70].replace("\n", " "))
        sys.exit(1)
    s = s.replace(old, new)


patch("#include <QGuiApplication>",
      "#include <QDir>\n#include <QFile>\n#include <QGuiApplication>")

patch("    LiquidGlassConfig::self()->read();\n\n    int blurStrength = LiquidGlassConfig::blurStrength() - 1;",
      """    LiquidGlassConfig::self()->read();

    const QString overrideFrag = QDir::homePath() + QStringLiteral("/.config/liquidglass/onscreen_rounded.frag");
    if (QFile::exists(overrideFrag)) {
        auto overrideShader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                                QStringLiteral(":/effects/liquidglass/shaders/onscreen_rounded.vert"),
                                                                                overrideFrag);
        if (overrideShader) {
            m_roundedOnscreenPass.shader = std::move(overrideShader);
            m_roundedOnscreenPass.mvpMatrixLocation = m_roundedOnscreenPass.shader->uniformLocation("modelViewProjectionMatrix");
            m_roundedOnscreenPass.colorMatrixLocation = m_roundedOnscreenPass.shader->uniformLocation("colorMatrix");
            m_roundedOnscreenPass.offsetLocation = m_roundedOnscreenPass.shader->uniformLocation("offset");
            m_roundedOnscreenPass.halfpixelLocation = m_roundedOnscreenPass.shader->uniformLocation("halfpixel");
            m_roundedOnscreenPass.boxLocation = m_roundedOnscreenPass.shader->uniformLocation("box");
            m_roundedOnscreenPass.cornerRadiusLocation = m_roundedOnscreenPass.shader->uniformLocation("cornerRadius");
            m_roundedOnscreenPass.opacityLocation = m_roundedOnscreenPass.shader->uniformLocation("opacity");
            m_roundedOnscreenPass.sharpLocation = m_roundedOnscreenPass.shader->uniformLocation("sharpTex");
            qCWarning(KWIN_LIQUIDGLASS) << "shader override loaded:" << overrideFrag;
        } else {
            qCWarning(KWIN_LIQUIDGLASS) << "shader override failed to compile, keeping the current shader";
        }
    }

    int blurStrength = LiquidGlassConfig::blurStrength() - 1;""")

open(c, "w").write(s)
print("patched")
