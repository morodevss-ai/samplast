package com.goldrp.game.common;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.RippleDrawable;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.ViewPropertyAnimator;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.LinearInterpolator;
import android.view.animation.RotateAnimation;
import android.view.animation.ScaleAnimation;

import java.util.Set;

import kotlin.NoWhenBranchMatchedException;
import kotlin.Unit;
import kotlin.jvm.functions.Function0;
import kotlin.jvm.internal.Intrinsics;
import kotlin.collections.SetsKt;

public final class EasyAnimation {
    public static final EasyAnimation INSTANCE = new EasyAnimation();

    private EasyAnimation() {
    }
    public void setOnClickListenerWithAnim(View view, Function0<Unit> function0) {
        Intrinsics.checkNotNullParameter(view, "<this>");
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                view.animate().scaleX(0.9f).scaleY(0.9f).setDuration(100)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                view.animate().scaleX(1.0f).scaleY(1.0f).setDuration(100)
                                        .withEndAction(new Runnable() {
                                            @Override
                                            public void run() {
                                                if (function0 != null) {
                                                    function0.invoke();
                                                }
                                            }
                                        });
                            }
                        });
            }
        });
    }
    // region ScaleAndFadeAnimation
    public void scaleAndFadeAnimation(View view, boolean reverse, long duration, Function0<Unit> onStart, Function0<Unit> onEnd) {
        Intrinsics.checkNotNullParameter(view, "view");
        view.setPivotX(view.getWidth() / 2.0f);
        view.setPivotY(view.getHeight() / 2.0f);

        float startScale = reverse ? 2.2f : 1.0f;
        float endScale = reverse ? 1.0f : 2.2f;
        float startAlpha = reverse ? 0.0f : 1.0f;
        float endAlpha = reverse ? 1.0f : 0.0f;

        ObjectAnimator scaleX = ObjectAnimator.ofFloat(view, "scaleX", startScale, endScale);
        ObjectAnimator scaleY = ObjectAnimator.ofFloat(view, "scaleY", startScale, endScale);
        ObjectAnimator alpha = ObjectAnimator.ofFloat(view, "alpha", startAlpha, endAlpha);

        AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.setInterpolator(new DecelerateInterpolator());
        animatorSet.setDuration(duration);
        animatorSet.playTogether(scaleX, scaleY, alpha);
        animatorSet.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
                if (onStart != null) {
                    onStart.invoke();
                }
            }
            @Override
            public void onAnimationEnd(Animator animation) {
                if (onEnd != null) {
                    onEnd.invoke();
                }
            }
            @Override
            public void onAnimationCancel(Animator animation) { }
            @Override
            public void onAnimationRepeat(Animator animation) { }
        });
        animatorSet.start();
    }
    // endregion

    // region AnimateVisible
    public void animateVisible(View view, boolean isVisible, Function0<Unit> onStart, Function0<Unit> onEnd, long duration) {
        Intrinsics.checkNotNullParameter(view, "view");
        view.clearAnimation();
        float targetAlpha = isVisible ? 1.0f : 0.0f;
        view.setAlpha(isVisible ? 0.0f : 1.0f);
        view.animate()
                .withStartAction(() -> {
                    if (onStart != null) {
                        onStart.invoke();
                    }
                })
                .alpha(targetAlpha)
                .setDuration(duration)
                .withEndAction(() -> {
                    if (onEnd != null) {
                        onEnd.invoke();
                    }
                })
                .start();
    }
    // endregion
    public static void animateClick$default(EasyAnimation easyAnimation, View view, long j, Function0 function0, Function0 function02, int i, Object obj) {
        if ((i & 1) != 0) {
            j = 30;
        }
        easyAnimation.animateClick(view, j, (i & 2) != 0 ? null : function0, (i & 4) != 0 ? null : function02);
    }
    // region AnimateClick
    public void animateClick(View view, long delay, Function0<Unit> onStart, Function0<Unit> onEnd) {
        Intrinsics.checkNotNullParameter(view, "view");
        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                v.clearAnimation();
                ScaleAnimation scaleAnimation = new ScaleAnimation(
                        1.0f, 0.95f,
                        1.0f, 0.95f,
                        ScaleAnimation.RELATIVE_TO_SELF, 0.5f,
                        ScaleAnimation.RELATIVE_TO_SELF, 0.5f);
                scaleAnimation.setDuration(delay);
                scaleAnimation.setRepeatCount(1);
                scaleAnimation.setRepeatMode(ScaleAnimation.REVERSE);
                scaleAnimation.setAnimationListener(new android.view.animation.Animation.AnimationListener() {
                    @Override
                    public void onAnimationStart(android.view.animation.Animation animation) {
                        if (onStart != null) {
                            onStart.invoke();
                        }
                    }
                    @Override
                    public void onAnimationEnd(android.view.animation.Animation animation) {
                        if (onEnd != null) {
                            onEnd.invoke();
                        }
                    }
                    @Override
                    public void onAnimationRepeat(android.view.animation.Animation animation) { }
                });
                v.startAnimation(scaleAnimation);
            }
        });
    }
    // endregion

    // region InfiniteRotation
    public void startInfiniteRotation(View view, long duration, boolean isInfinite) {
        Intrinsics.checkNotNullParameter(view, "view");
        RotateAnimation rotateAnimation = new RotateAnimation(
                0.0f, 360.0f,
                RotateAnimation.RELATIVE_TO_SELF, 0.5f,
                RotateAnimation.RELATIVE_TO_SELF, 0.5f);
        rotateAnimation.setDuration(duration);
        if (isInfinite) {
            rotateAnimation.setRepeatCount(RotateAnimation.INFINITE);
        }
        rotateAnimation.setInterpolator(new LinearInterpolator());
        view.startAnimation(rotateAnimation);
    }
    // endregion

    // region SlideLeftWithFadeOutAnimation
    public void slideLeftWithFadeOutAnimation(View view, Float translation, long duration, Function0<Unit> onEnd) {
        Intrinsics.checkNotNullParameter(view, "view");
        float targetTranslation = translation != null ? translation : -((float) view.getWidth()) / 3.0f;
        view.animate()
                .translationX(targetTranslation)
                .alpha(0.0f)
                .setInterpolator(new AccelerateDecelerateInterpolator())
                .setDuration(duration)
                .withEndAction(() -> {
                    view.setTranslationX(0.0f);
                    view.setAlpha(1.0f);
                    if (onEnd != null) {
                        onEnd.invoke();
                    }
                })
                .start();
    }
    // endregion

    // region AnimateOpacityAndTranslationX
    public void animateOpacityAndTranslationX(View view, Float translation, boolean isVisible, int factor, long duration, Function0<Unit> onStart, Function0<Unit> onEnd) {
        Intrinsics.checkNotNullParameter(view, "view");
        float defaultTranslation = translation != null ? translation : ((float) view.getWidth()) / 3.0f;
        view.setAlpha(isVisible ? 0.0f : 1.0f);
        view.setTranslationX(isVisible ? factor * defaultTranslation : 0.0f);
        ViewPropertyAnimator animator = view.animate().translationX(isVisible ? 0.0f : factor * defaultTranslation);
        animator.alpha(isVisible ? 1.0f : 0.0f)
                .setInterpolator(new AccelerateDecelerateInterpolator())
                .setDuration(duration)
                .withStartAction(() -> {
                    if (onStart != null) {
                        onStart.invoke();
                    }
                })
                .withEndAction(() -> {
                    if (onEnd != null) {
                        onEnd.invoke();
                    }
                })
                .start();
    }
    // endregion
    public static void slideWithFade$default(EasyAnimation easyAnimation, View view, SlideDirection slideDirection, long j, Function0 function0, Function0 function02, int i, Object obj) {
        if ((i & 2) != 0) {
            j = 300;
        }
        easyAnimation.slideWithFade(view, slideDirection, j, (i & 4) != 0 ? null : function0, (i & 8) != 0 ? null : function02);
    }
    // region SlideWithFade
    public static void slideWithFade(View view, SlideDirection direction, long duration, Function0<Unit> onStart, Function0<Unit> onEnd) {
        Intrinsics.checkNotNullParameter(view, "view");
        Intrinsics.checkNotNullParameter(direction, "direction");
        ObjectAnimator translationAnimator;
        switch (direction) {
            case TOP_TO_BOTTOM:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationY", -view.getHeight(), 0.0f);
                break;
            case BOTTOM_TO_TOP:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationY", view.getHeight(), 0.0f);
                break;
            case LEFT_TO_RIGHT:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationX", -view.getWidth(), 0.0f);
                break;
            case RIGHT_TO_LEFT:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationX", view.getWidth(), 0.0f);
                break;
            case OUT_SCREEN_DOWN:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationY", 0.0f, 2 * view.getHeight());
                break;
            case OUT_SCREEN_LEFT:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationX", 0.0f, -view.getWidth());
                break;
            case OUT_SCREEN_RIGHT:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationX", 0.0f, view.getWidth());
                break;
            case OUT_SCREEN_UP:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationY", 0.0f, -view.getHeight());
                break;
            case SCREEN_TO_DEFAULT:
                translationAnimator = ObjectAnimator.ofFloat(view, "translationY", view.getHeight(), 0.0f);
                break;
            default:
                throw new NoWhenBranchMatchedException();
        }

        ObjectAnimator alphaAnimator;
        Set<SlideDirection> set = SetsKt.setOf(SlideDirection.OUT_SCREEN_DOWN, SlideDirection.OUT_SCREEN_LEFT, SlideDirection.OUT_SCREEN_RIGHT, SlideDirection.OUT_SCREEN_UP);
        if (set.contains(direction)) {
            alphaAnimator = ObjectAnimator.ofFloat(view, "alpha", 1.0f, 0.0f);
        } else {
            alphaAnimator = ObjectAnimator.ofFloat(view, "alpha", 0.0f, 1.0f);
        }

        AnimatorSet animatorSet = new AnimatorSet();
        animatorSet.playTogether(translationAnimator, alphaAnimator);
        animatorSet.setDuration(duration);
        animatorSet.addListener(new Animator.AnimatorListener() {
            @Override
            public void onAnimationStart(Animator animation) {
                if (onStart != null) {
                    onStart.invoke();
                }
            }
            @Override
            public void onAnimationEnd(Animator animation) {
                if (onEnd != null) {
                    onEnd.invoke();
                }
            }
            @Override
            public void onAnimationCancel(Animator animation) { }
            @Override
            public void onAnimationRepeat(Animator animation) { }
        });
        animatorSet.start();
    }
    // endregion

    // region AddRippleEffect
    public void addRippleEffect(View view, int rippleColor, long rippleDuration, Function0<Unit> onClick) {
        Intrinsics.checkNotNullParameter(view, "view");
        view.setOnClickListener(v -> {
            RippleDrawable rippleDrawable = new RippleDrawable(ColorStateList.valueOf(rippleColor), view.getBackground(), null);
            view.setBackground(rippleDrawable);
            rippleDrawable.setState(new int[]{16842919, 16842910});
            new Handler(Looper.getMainLooper()).postDelayed(() -> {
                rippleDrawable.setState(new int[0]);
                if (onClick != null) {
                    onClick.invoke();
                }
            }, rippleDuration);
        });
    }
    // endregion

    // Nested enum SlideDirection
    public enum SlideDirection {
        TOP_TO_BOTTOM,
        BOTTOM_TO_TOP,
        LEFT_TO_RIGHT,
        RIGHT_TO_LEFT,
        OUT_SCREEN_DOWN,
        OUT_SCREEN_LEFT,
        OUT_SCREEN_RIGHT,
        OUT_SCREEN_UP,
        SCREEN_TO_DEFAULT;
    }

    public void HideLayout(View view) {
        view.animate().alpha(0.0f).setDuration(150).withEndAction(() -> {});
    }

    public void HideLayout(View view, Function0<Unit> function0) {
        view.animate().alpha(0.0f).setDuration(150).withEndAction(() -> {
            if (function0 != null) {
                function0.invoke();
            }
        });
    }

    public void ShowLayout(View view) {
        view.setVisibility(View.VISIBLE);
        view.setAlpha(0.0f);
        view.animate().alphaBy(1.0f).setDuration(150).start();
    }

    public void ShowLayout(View view, Function0<Unit> function0) {
        view.setVisibility(View.VISIBLE);
        view.animate().alphaBy(1.0f).setDuration(150).withEndAction(() -> {
            if (function0 != null) {
                function0.invoke();
            }
        }).start();
    }
}