import sys

base = sys.argv[1]
h = base + "/liquidglass.h"
c = base + "/liquidglass.cpp"


def patch(path, old, new):
    s = open(path).read()
    if s.count(old) != 1:
        print("ANCHOR NOT FOUND OR NOT UNIQUE in " + path + ": " + old[:70].replace("\n", " "))
        sys.exit(1)
    open(path, "w").write(s.replace(old, new))


hs = open(h).read()
cs = open(c).read()
if "sharpLocation" in hs and "sharpLocation" in cs:
    print("already patched")
    sys.exit(0)

patch(h,
      "        int opacityLocation;\n    } m_roundedOnscreenPass;",
      "        int opacityLocation;\n        int sharpLocation;\n    } m_roundedOnscreenPass;")

patch(c,
      "#include \"opengl/glplatform.h\"",
      "#include \"opengl/glplatform.h\"\n#include <epoxy/gl.h>")

patch(c,
      "m_roundedOnscreenPass.opacityLocation = m_roundedOnscreenPass.shader->uniformLocation(\"opacity\");",
      "m_roundedOnscreenPass.opacityLocation = m_roundedOnscreenPass.shader->uniformLocation(\"opacity\");\n        m_roundedOnscreenPass.sharpLocation = m_roundedOnscreenPass.shader->uniformLocation(\"sharpTex\");")

patch(c,
      "m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.opacityLocation, modulation);\n\n        read->colorAttachment()->bind();",
      "m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.opacityLocation, modulation);\n        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.sharpLocation, 1);\n\n        read->colorAttachment()->bind();\n        glActiveTexture(GL_TEXTURE1);\n        renderInfo.framebuffers[0]->colorAttachment()->bind();\n        glActiveTexture(GL_TEXTURE0);")

patch(c,
      "        glDisable(GL_BLEND);\n\n        ShaderManager::instance()->popShader();\n    } else {",
      "        glDisable(GL_BLEND);\n\n        glActiveTexture(GL_TEXTURE1);\n        glBindTexture(GL_TEXTURE_2D, 0);\n        glActiveTexture(GL_TEXTURE0);\n\n        ShaderManager::instance()->popShader();\n    } else {")

print("patched")
