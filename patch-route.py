import sys

base = sys.argv[1]
c = base + "/liquidglass.cpp"
s = open(c).read()
if "useWindowGeometry" in s:
    print("already patched")
    sys.exit(0)
if "sharpLocation" not in s:
    print("run the earlier patches first (sharpLocation missing)")
    sys.exit(1)


def patch(old, new):
    global s
    if s.count(old) != 1:
        print("ANCHOR NOT FOUND OR NOT UNIQUE: " + old[:70].replace("\n", " "))
        sys.exit(1)
    s = s.replace(old, new)


patch("    if (const BorderRadius cornerRadius = w->window()->borderRadius(); !cornerRadius.isNull()) {",
      """    const BorderRadius cornerRadius = w->window()->borderRadius();
    const bool useWindowGeometry = !cornerRadius.isNull();
    if (true) {""")

patch("""        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.boxLocation, QVector4D(nativeBox.horizontalCenter(), nativeBox.verticalCenter(), nativeBox.width() * 0.5, nativeBox.height() * 0.5));
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.cornerRadiusLocation, nativeCornerRadius.toVector());""",
      """        QVector4D boxUniform(nativeBox.horizontalCenter(), nativeBox.verticalCenter(), nativeBox.width() * 0.5, nativeBox.height() * 0.5);
        QVector4D radiusUniform = nativeCornerRadius.toVector();
        if (!useWindowGeometry) {
            const float bw = float(scaledBackgroundRect.width());
            const float bh = float(scaledBackgroundRect.height());
            float rr = 14.0f * float(viewport.scale());
            if (rr > bw * 0.5f) {
                rr = bw * 0.5f;
            }
            if (rr > bh * 0.5f) {
                rr = bh * 0.5f;
            }
            boxUniform = QVector4D(bw * 0.5f, bh * 0.5f, bw * 0.5f, bh * 0.5f);
            radiusUniform = QVector4D(rr, rr, rr, rr);
        }
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.boxLocation, boxUniform);
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.cornerRadiusLocation, radiusUniform);""")

open(c, "w").write(s)
print("patched")
