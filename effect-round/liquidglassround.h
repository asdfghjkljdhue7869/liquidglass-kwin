#pragma once

#include "effect/offscreeneffect.h"

#include <QString>
#include <QVector4D>
#include <array>
#include <chrono>
#include <QPointF>
#include <memory>
#include <unordered_map>

namespace KWin
{

class GLShader;

class LiquidGlassRoundEffect : public OffscreenEffect
{
    Q_OBJECT

public:
    LiquidGlassRoundEffect();
    ~LiquidGlassRoundEffect() override;

    static bool supported();
    static bool enabledByDefault();

    void reconfigure(ReconfigureFlags flags) override;
    void prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data) override;
    bool drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data) override;
    void postPaintScreen() override;
    bool isActive() const override;

    int requestedEffectChainPosition() const override
    {
        return 75;
    }

public Q_SLOTS:
    void slotWindowAdded(KWin::EffectWindow *w);
    void slotWindowDeleted(KWin::EffectWindow *w);

private:
    int kindOf(EffectWindow *w) const;
    void updateWindow(EffectWindow *w);
    void readConfig();

    std::unique_ptr<GLShader> m_shader;
    std::unordered_map<EffectWindow *, int> m_redirected;
    qreal m_radius = 25.0;
    qreal m_menuRadius = 18.0;
    qreal m_dockRadius = 24.0;
    qreal m_dockInset = 10.0;
    bool m_roundTooltips = false;
    qreal m_shadowStrength = 0.0;
    qreal m_shadowSize = 12.0;
    qreal m_shadowOffset = 3.0;
    bool m_shadowActive = false;
    QString m_keyClass;
    QVector4D m_keyColor = QVector4D(0.0f, 0.0f, 0.0f, 0.05f);
    float m_keyMin = 0.0f;
    struct DropState {
        bool started = false;
        std::chrono::steady_clock::time_point added;
        bool quick = false;
        bool hasStart = false;
        double startRect[4] = {0.0, 0.0, 0.0, 0.0};
        int frames = 0;
        std::chrono::steady_clock::time_point start;
        QPointF anchor;
        double rect[4] = {0.0, 0.0, 0.0, 0.0};
        double radv[4] = {0.0, 0.0, 0.0, 0.0};
        double fade = 1.0;
    };
    std::unordered_map<EffectWindow *, DropState> m_drops;
    int m_dropMs = 0;
    double m_dropR0 = 7.0;
    double m_dropLag = 0.35;
    double m_dropHold = 0.55;
    double m_dropFade0 = 0.10;
    double m_dropFade1 = 0.55;
    int m_dropQuickMs = 240;
    int m_dropChainMs = 900;
    std::chrono::steady_clock::time_point m_lastPopupEvent;
    std::chrono::steady_clock::time_point m_lastCloseTime;
    double m_lastRect[4] = {0.0, 0.0, 0.0, 0.0};
    bool m_lastRectValid = false;
    bool m_loading = true;
    void tickDrop(EffectWindow *w);
    std::unordered_map<EffectWindow *, std::array<double, 4>> m_innerRects;
};

}
