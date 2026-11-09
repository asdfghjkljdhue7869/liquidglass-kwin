#include "liquidglassround.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "effect/effectwindow.h"
#include "effect/globals.h"
#include "opengl/glshader.h"
#include "opengl/glshadermanager.h"
#include "window.h"
#include "core/region.h"
#include "wayland/surface.h"

#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QPointer>
#include <QStringList>
#include <QVector4D>

#include <algorithm>
#include <cmath>

#include "roundshader.h"

static constexpr int kDropRectRole = 5101;
static constexpr int kDropRadiusRole = 5102;

Q_LOGGING_CATEGORY(KWIN_LIQUIDGLASSROUND, "kwin_effect_liquidglassround", QtWarningMsg)

namespace KWin
{

static qreal readFrameRadius(qreal fallback)
{
    QFile file(QDir::homePath() + QStringLiteral("/.config/liquidglass/frame_radius"));
    if (file.open(QIODevice::ReadOnly)) {
        bool ok = false;
        const double value = QString::fromUtf8(file.readAll()).trimmed().toDouble(&ok);
        if (ok && value >= 0.0 && value <= 200.0) {
            return value;
        }
    }
    return fallback;
}

static void logKind(int kind, const char *what, EffectWindow *w)
{
    static int otherLogs = 0;
    if (kind == 1 || otherLogs++ < 200) {
        qCWarning(KWIN_LIQUIDGLASSROUND) << "round:" << what << kind << w;
    }
}

LiquidGlassRoundEffect::LiquidGlassRoundEffect()
    : OffscreenEffect()
{
    readConfig();
    connect(effects, &EffectsHandler::windowAdded, this, &LiquidGlassRoundEffect::slotWindowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &LiquidGlassRoundEffect::slotWindowDeleted);
    const auto windows = effects->stackingOrder();
    for (EffectWindow *w : windows) {
        slotWindowAdded(w);
    }
    m_loading = false;
    qCWarning(KWIN_LIQUIDGLASSROUND) << "round: loaded, radius" << m_radius;
}

LiquidGlassRoundEffect::~LiquidGlassRoundEffect()
{
    effects->makeOpenGLContextCurrent();
    for (const auto &entry : m_redirected) {
        unredirect(entry.first);
    }
    m_redirected.clear();
    for (const auto &entry : m_drops) {
        entry.first->setData(kDropRectRole, QVariant());
        entry.first->setData(kDropRadiusRole, QVariant());
    }
    m_drops.clear();
    m_shader.reset();
}

bool LiquidGlassRoundEffect::supported()
{
    return OffscreenEffect::supported();
}

bool LiquidGlassRoundEffect::enabledByDefault()
{
    return false;
}

void LiquidGlassRoundEffect::readConfig()
{
    m_radius = readFrameRadius(m_radius);
    m_shadowActive = true;
    m_keyClass.clear();
    m_dropMs = 0;
    m_dropLag = 0.35;
    m_dropHold = 0.55;
    m_dropFade0 = 0.10;
    m_dropFade1 = 0.55;
    m_dropR0 = 7.0;
    QFile file(QDir::homePath() + QStringLiteral("/.config/liquidglass/round.conf"));
    if (file.open(QIODevice::ReadOnly)) {
        const QStringList rows = QString::fromUtf8(file.readAll()).split(QChar(10), Qt::SkipEmptyParts);
        for (const QString &row : rows) {
            const QStringList parts = row.simplified().split(QChar(32), Qt::SkipEmptyParts);
            if (parts.size() < 2) {
                continue;
            }
            if (parts[0] == QStringLiteral("menu")) {
                m_menuRadius = std::clamp(parts[1].toDouble(), 0.0, 100.0);
            } else if (parts[0] == QStringLiteral("dock")) {
                m_dockRadius = std::clamp(parts[1].toDouble(), 0.0, 100.0);
                if (parts.size() >= 3) {
                    m_dockInset = std::clamp(parts[2].toDouble(), 0.0, 40.0);
                }
            } else if (parts[0] == QStringLiteral("tooltips")) {
                m_roundTooltips = (parts[1] == QStringLiteral("1"));
            } else if (parts[0] == QStringLiteral("drop")) {
                m_dropMs = std::clamp(parts[1].toInt(), 0, 1500);
                if (parts.size() >= 3) {
                    m_dropR0 = std::clamp(parts[2].toDouble(), 5.0, 40.0);
                    if (parts.size() >= 4) {
                        m_dropLag = std::clamp(parts[3].toDouble(), 0.0, 0.8);
                    }
                    if (parts.size() >= 5) {
                        m_dropHold = std::clamp(parts[4].toDouble(), 0.0, 0.9);
                    }
                    if (parts.size() >= 6) {
                        m_dropFade0 = std::clamp(parts[5].toDouble(), 0.0, 0.9);
                    }
                    if (parts.size() >= 7) {
                        m_dropFade1 = std::clamp(parts[6].toDouble(), m_dropFade0 + 0.05, 1.0);
                    }
                }
            } else if (parts[0] == QStringLiteral("key") && parts.size() >= 7) {
                m_keyClass = parts[1];
                m_keyColor = QVector4D(float(std::clamp(parts[2].toDouble() / 255.0, 0.0, 1.0)), float(std::clamp(parts[3].toDouble() / 255.0, 0.0, 1.0)), float(std::clamp(parts[4].toDouble() / 255.0, 0.0, 1.0)), float(std::clamp(parts[5].toDouble(), 0.0, 1.0)));
                m_keyMin = float(std::clamp(parts[6].toDouble(), 0.0, 1.0));
            } else if (parts[0] == QStringLiteral("shadow")) {
                m_shadowActive = true;
                m_shadowStrength = std::clamp(parts[1].toDouble(), 0.0, 1.0);
                if (parts.size() >= 3) {
                    m_shadowSize = std::clamp(parts[2].toDouble(), 1.0, 60.0);
                }
                if (parts.size() >= 4) {
                    m_shadowOffset = std::clamp(parts[3].toDouble(), -20.0, 40.0);
                }
            }
        }
    }
    qCWarning(KWIN_LIQUIDGLASSROUND) << "round: settings frame" << m_radius << "menu" << m_menuRadius << "dock" << m_dockRadius << "inset" << m_dockInset << "tooltips" << m_roundTooltips << "shadow" << m_shadowStrength << m_shadowSize << m_shadowOffset << "key" << m_keyClass << m_keyColor << m_keyMin << "drop" << m_dropMs << m_dropR0 << m_dropLag << m_dropHold << m_dropFade0 << m_dropFade1;
}

void LiquidGlassRoundEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags)
    readConfig();
    const auto windows = effects->stackingOrder();
    for (EffectWindow *w : windows) {
        updateWindow(w);
    }
    effects->addRepaintFull();
}

bool LiquidGlassRoundEffect::isActive() const
{
    return !m_redirected.empty();
}

int LiquidGlassRoundEffect::kindOf(EffectWindow *w) const
{
    if (!w || w->isDeleted() || !w->windowItem()) {
        return 0;
    }
    Window *win = w->window();
    if (!win || w->isDesktop()) {
        return 0;
    }
    if (w->isDock()) {
        const RectF area = effects->clientArea(ScreenArea, w);
        return w->frameGeometry().width() < area.width() - 2.0 ? 3 : 0;
    }
    if (w->isTooltip()) {
        return m_roundTooltips ? 2 : 0;
    }
    if (w->hasDecoration()) {
        if (w->isFullScreen() || win->maximizeMode() != MaximizeRestore || win->tile() || win->requestedTile()) {
            return 0;
        }
        return 1;
    }
    if (w->isPopupWindow() || w->isPopupMenu() || w->isDropdownMenu() || w->isComboBox() || w->isMenu()) {
        return 2;
    }
    if (w->isSpecialWindow() && win->resourceClass().contains(QStringLiteral("plasmashell"))) {
        return 2;
    }
    return 0;
}

void LiquidGlassRoundEffect::updateWindow(EffectWindow *w)
{
    const int kind = kindOf(w);
    const auto it = m_redirected.find(w);
    const bool have = it != m_redirected.end();
    if (kind != 0 && !have) {
        redirect(w);
        m_redirected[w] = kind;
        logKind(kind, "redirected", w);
    } else if (kind == 0 && have) {
        unredirect(w);
        m_redirected.erase(w);
        logKind(1, "unredirected", w);
    } else if (kind != 0 && have && it->second != kind) {
        m_redirected[w] = kind;
    } else {
        return;
    }
    effects->addRepaintFull();
}

void LiquidGlassRoundEffect::slotWindowAdded(EffectWindow *w)
{
    if (m_dropMs > 0 && !m_loading && !w->isTooltip() && (w->isPopupWindow() || w->isPopupMenu() || w->isDropdownMenu() || w->isMenu() || w->isComboBox()) && kindOf(w) == 2) {
        DropState st;
        st.anchor = effects->cursorPos();
        st.added = std::chrono::steady_clock::now();
        m_drops[w] = st;
    }
    QPointer<EffectWindow> guard(w);
    auto refresh = [this, guard]() {
        if (guard) {
            updateWindow(guard.data());
        }
    };
    if (Window *win = w->window()) {
        connect(win, &Window::maximizedChanged, this, refresh);
        connect(win, &Window::fullScreenChanged, this, refresh);
        connect(win, &Window::tileChanged, this, refresh);
        connect(win, &Window::requestedTileChanged, this, refresh);
        connect(win, &Window::decorationChanged, this, refresh);
    }
    connect(w, &EffectWindow::windowDecorationChanged, this, refresh);
    updateWindow(w);
}

void LiquidGlassRoundEffect::slotWindowDeleted(EffectWindow *w)
{
    m_redirected.erase(w);
    m_innerRects.erase(w);
    m_drops.erase(w);
}

void LiquidGlassRoundEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data)
{
    if (m_redirected.contains(w)) {
        data.setTranslucent();
    }
    tickDrop(w);
    effects->prePaintWindow(view, w, data);
}

void LiquidGlassRoundEffect::postPaintScreen()
{
    for (const auto &entry : m_drops) {
        effects->addRepaint(entry.first->expandedGeometry());
    }
    effects->postPaintScreen();
}

void LiquidGlassRoundEffect::tickDrop(EffectWindow *w)
{
    const auto it = m_drops.find(w);
    if (it == m_drops.end()) {
        return;
    }
    DropState &st = it->second;
    const auto now = std::chrono::steady_clock::now();
    if (!st.started) {
        st.started = true;
        st.start = now;
        static int dropLogs = 0;
        if (dropLogs++ < 12) {
            const double waited = std::chrono::duration<double, std::milli>(now - st.added).count();
            qCWarning(KWIN_LIQUIDGLASSROUND) << "round: drop start anchor" << st.anchor << "ms" << m_dropMs << "waitedForFirstPaint" << waited;
        }
    }
    st.frames++;
    const double elapsed = std::chrono::duration<double, std::milli>(now - st.start).count();
    const double t = elapsed / double(std::max(m_dropMs, 1));
    if (t >= 1.0 || !m_redirected.contains(w)) {
        static int endLogs = 0;
        if (endLogs++ < 12) {
            qCWarning(KWIN_LIQUIDGLASSROUND) << "round: drop end frames" << st.frames << "elapsedMs" << elapsed;
        }
        w->setData(kDropRectRole, QVariant());
        w->setData(kDropRadiusRole, QVariant());
        m_drops.erase(it);
        return;
    }
    const RectF frame = w->frameGeometry();
    RectF inner(0.0, 0.0, frame.width(), frame.height());
    if (SurfaceInterface *surf = w->surface()) {
        const RegionF blurReg = surf->blurRegion();
        if (!blurReg.isEmpty()) {
            inner = blurReg.boundingRect().intersected(inner);
        }
    }
    const double ax = std::clamp(st.anchor.x() - frame.x(), inner.left(), inner.right());
    const double ay = std::clamp(st.anchor.y() - frame.y(), inner.top(), inner.bottom());
    double dx = (inner.left() + inner.right()) * 0.5 - ax;
    double dy = (inner.top() + inner.bottom()) * 0.5 - ay;
    const double len = std::hypot(dx, dy);
    if (len > 1e-3) {
        dx /= len;
        dy /= len;
    } else {
        dx = 0.0;
        dy = 0.0;
    }
    auto lead = [&](double nx, double ny) {
        return std::clamp(0.5 + 0.5 * (nx * dx + ny * dy), 0.0, 1.0);
    };
    auto edge = [&](double leadValue) {
        const double d = m_dropLag * (1.0 - leadValue);
        const double te = std::clamp((t - d) / (1.0 - d), 0.0, 1.0);
        return 1.0 - std::pow(1.0 - te, 3.0);
    };
    const double sl = ax - m_dropR0;
    const double stp = ay - m_dropR0;
    const double sr = ax + m_dropR0;
    const double sb = ay + m_dropR0;
    const double l = sl + (inner.left() - sl) * edge(lead(-1.0, 0.0));
    const double tp = stp + (inner.top() - stp) * edge(lead(0.0, -1.0));
    const double r = sr + (inner.right() - sr) * edge(lead(1.0, 0.0));
    const double b = sb + (inner.bottom() - sb) * edge(lead(0.0, 1.0));
    const double nxs[4] = {-1.0, 1.0, -1.0, 1.0};
    const double nys[4] = {-1.0, -1.0, 1.0, 1.0};
    const double finalRadius = std::min(m_menuRadius, std::min(inner.width(), inner.height()) * 0.5);
    const double pill = std::min(r - l, b - tp) * 0.5;
    for (int c = 0; c < 4; ++c) {
        const double lc = lead(nxs[c] * 0.70710678, nys[c] * 0.70710678);
        const double hold = m_dropHold * (0.4 + 0.6 * lc);
        const double rt = std::clamp((t - hold) / std::max(1.0 - hold, 0.05), 0.0, 1.0);
        const double sm = rt * rt * (3.0 - 2.0 * rt);
        st.radv[c] = std::max(0.0, pill + (finalRadius - pill) * sm);
    }
    st.rect[0] = l;
    st.rect[1] = tp;
    st.rect[2] = r;
    st.rect[3] = b;
    const double ft = std::clamp((t - m_dropFade0) / std::max(m_dropFade1 - m_dropFade0, 0.05), 0.0, 1.0);
    st.fade = ft * ft * (3.0 - 2.0 * ft);
    w->setData(kDropRectRole, QRectF(frame.x() + l, frame.y() + tp, r - l, b - tp));
    w->setData(kDropRadiusRole, QVariant::fromValue(QVector4D(float(st.radv[0]), float(st.radv[1]), float(st.radv[2]), float(st.radv[3]))));
}

bool LiquidGlassRoundEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data)
{
    const auto it = m_redirected.find(w);
    if (it != m_redirected.end()) {
        if (!m_shader) {
            const ShaderTraits traits = ShaderTrait::MapTexture | ShaderTrait::Modulate | ShaderTrait::AdjustSaturation | ShaderTrait::TransformColorspace | ShaderTrait::RoundedCorners;
            m_shader = ShaderManager::instance()->generateCustomShader(traits, QByteArray(), QByteArray(kRoundFragmentSource));
            if (m_shader) {
                qCWarning(KWIN_LIQUIDGLASSROUND) << "round: shadow-safe shader created";
            } else {
                m_shader = ShaderManager::instance()->generateCustomShader(traits);
                qCWarning(KWIN_LIQUIDGLASSROUND) << (m_shader ? "round: shadow-safe shader FAILED, using the stock one" : "round: all shaders FAILED");
            }
        }
        if (m_shader) {
            setShader(w, m_shader.get());
            const int kind = it->second;
            const double scale = viewport.scale();
            const RectF frame = w->frameGeometry();
            const double bw = frame.width() * scale;
            const double bh = frame.height() * scale;
            double radius = m_radius;
            double inset = 0.0;
            if (kind == 2) {
                radius = m_menuRadius;
            } else if (kind == 3) {
                radius = m_dockRadius;
                inset = m_dockInset;
            }
            const double ex = bw * 0.5 - inset * scale;
            const double ey = bh * 0.5 - inset * scale;
            const double rad = std::max(0.0, std::min(radius * scale, std::min(ex, ey)));
            ShaderBinder binder(m_shader.get());
            m_shader->setUniform(GLShader::Vec4Uniform::Box, QVector4D(bw * 0.5, bh * 0.5, ex, ey));
            m_shader->setUniform(GLShader::Vec4Uniform::CornerRadius, QVector4D(rad, rad, rad, rad));
            double dropFade = 1.0;
            bool innerMode = false;
            if (m_shadowActive) {
                RectF inner(0.0, 0.0, frame.width(), frame.height());
                bool haveInner = false;
                SurfaceInterface *surf = (kind == 1) ? nullptr : w->surface();
                if (surf) {
                    const RegionF blurReg = surf->blurRegion();
                    if (!blurReg.isEmpty()) {
                        inner = blurReg.boundingRect().intersected(inner);
                        m_innerRects[w] = {inner.x(), inner.y(), inner.width(), inner.height()};
                        haveInner = true;
                    }
                }
                if (!haveInner) {
                    const auto cached = m_innerRects.find(w);
                    if (cached != m_innerRects.end()) {
                        inner = RectF(cached->second[0], cached->second[1], cached->second[2], cached->second[3]);
                    }
                }
                double dropRadius = -1.0;
                double dropRad4[4] = {0.0, 0.0, 0.0, 0.0};
                bool dropHas4 = false;
                {
                    const auto dit = m_drops.find(w);
                    if (dit != m_drops.end() && dit->second.started) {
                        inner = RectF(dit->second.rect[0], dit->second.rect[1], dit->second.rect[2] - dit->second.rect[0], dit->second.rect[3] - dit->second.rect[1]);
                        dropRadius = dit->second.radv[0];
                        dropHas4 = true;
                        for (int ci = 0; ci < 4; ++ci) {
                            dropRad4[ci] = dit->second.radv[ci];
                        }
                        dropFade = dit->second.fade;
                    }
                }
                if (inner.width() > 8.0 && inner.height() > 8.0) {
                    const double hx = inner.width() * 0.5 * scale;
                    const double hy = inner.height() * 0.5 * scale;
                    const double cx = (inner.x() + inner.width() * 0.5) * scale;
                    const double cy = (inner.y() + inner.height() * 0.5) * scale;
                    const double r2 = std::max(0.0, std::min((dropRadius >= 0.0 ? dropRadius : radius) * scale, std::min(hx, hy)));
                    m_shader->setUniform(GLShader::Vec4Uniform::Box, QVector4D(cx, cy, hx, hy));
                    const double cap = std::min(hx, hy);
                    if (dropHas4) {
                        m_shader->setUniform(GLShader::Vec4Uniform::CornerRadius, QVector4D(float(std::min(dropRad4[0] * scale, cap)), float(std::min(dropRad4[1] * scale, cap)), float(std::min(dropRad4[2] * scale, cap)), float(std::min(dropRad4[3] * scale, cap))));
                    } else {
                        m_shader->setUniform(GLShader::Vec4Uniform::CornerRadius, QVector4D(r2, r2, r2, r2));
                    }
                    innerMode = true;
                    static int innerLogs = 0;
                    if (innerLogs++ < 40) {
                        qCWarning(KWIN_LIQUIDGLASSROUND) << "round: inner rect kind" << kind << "fromSurface" << haveInner << "frame" << frame.width() << frame.height() << "inner" << inner.x() << inner.y() << inner.width() << inner.height();
                    }
                }
            }
            const double shadowOn = innerMode ? m_shadowStrength : 0.0;
            m_shader->setUniform("shadowParams", QVector4D(float(shadowOn), float(m_shadowSize * scale), float(m_shadowOffset * scale), innerMode ? 1.0f : 0.0f));
            bool keyOn = false;
            if (kind == 2 && !m_keyClass.isEmpty() && w->window() && w->window()->resourceClass().contains(m_keyClass, Qt::CaseInsensitive)) {
                keyOn = true;
                static int keyLogs = 0;
                if (keyLogs++ < 5) {
                    qCWarning(KWIN_LIQUIDGLASSROUND) << "round: key applied to" << w->window()->resourceClass();
                }
            }
            m_shader->setUniform("keyColor", m_keyColor);
            m_shader->setUniform("keyParams", QVector4D(keyOn ? 1.0f : 0.0f, m_keyMin, float(dropFade), 0.0f));
            static int marginLogs = 0;
            static int frameMarginLogs = 0;
            if (kind == 1 && frameMarginLogs++ < 8) {
                const auto ex2 = w->expandedGeometry();
                qCWarning(KWIN_LIQUIDGLASSROUND) << "round: shadow margins kind" << kind << "left" << frame.left() - ex2.left() << "top" << frame.top() - ex2.top() << "right" << ex2.right() - frame.right() << "bottom" << ex2.bottom() - frame.bottom();
            }
        }
    }
    return OffscreenEffect::drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);
}

}
