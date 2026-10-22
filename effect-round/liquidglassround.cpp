#include "liquidglassround.h"

#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "effect/effectwindow.h"
#include "effect/globals.h"
#include "opengl/glshader.h"
#include "opengl/glshadermanager.h"
#include "window.h"

#include <QDir>
#include <QFile>
#include <QLoggingCategory>
#include <QPointer>
#include <QStringList>
#include <QVector4D>

#include <algorithm>

#include "roundshader.h"

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
    if (kind == 1 || otherLogs++ < 12) {
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
    qCWarning(KWIN_LIQUIDGLASSROUND) << "round: loaded, radius" << m_radius;
}

LiquidGlassRoundEffect::~LiquidGlassRoundEffect()
{
    effects->makeOpenGLContextCurrent();
    for (const auto &entry : m_redirected) {
        unredirect(entry.first);
    }
    m_redirected.clear();
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
            }
        }
    }
    qCWarning(KWIN_LIQUIDGLASSROUND) << "round: settings frame" << m_radius << "menu" << m_menuRadius << "dock" << m_dockRadius << "inset" << m_dockInset << "tooltips" << m_roundTooltips;
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
}

void LiquidGlassRoundEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data)
{
    if (m_redirected.contains(w)) {
        data.setTranslucent();
    }
    effects->prePaintWindow(view, w, data);
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
        }
    }
    return OffscreenEffect::drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);
}

}
