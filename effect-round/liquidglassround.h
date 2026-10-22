#pragma once

#include "effect/offscreeneffect.h"

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
};

}
