package com.goldrp.game.common.views;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class KeyboardButtonShiftView extends View implements View.OnTouchListener {
    int colorArrow = Color.parseColor("#FFFFFF");

    public KeyboardButtonShiftView(Context context) {
        super(context);
        goInit();
    }

    public KeyboardButtonShiftView(Context context, AttributeSet attributeSet, int i) {
        super(context, attributeSet, i);
        goInit();
    }

    public KeyboardButtonShiftView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
        goInit();
    }

    public void goInit() {
        setOnTouchListener(this);
    }

    public boolean onTouch(View view, MotionEvent motionEvent) {
        if (motionEvent.getAction() == 0) {
            setBackgroundColor(-683726);
        }
        if (motionEvent.getAction() == 1) {
            setBackgroundColor(0);
        }
        return false;
    }

    public void setColorArrow(int i) {
        this.colorArrow = i;
        invalidate();
    }

    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        Path path = new Path();
        Paint paint = new Paint();
        int width = getWidth();
        int height = getHeight();
        path.reset();
        double d = (double) width;
        double d2 = (double) height;
        path.moveTo((float) (d * 0.5d), (float) (0.3d * d2));
        float f = (float) (0.5d * d2);
        path.lineTo((float) (0.35d * d), f);
        path.lineTo((float) (0.65d * d), f);
        float f2 = (float) (0.425d * d);
        path.moveTo(f2, f);
        float f3 = (float) (d2 * 0.7d);
        path.lineTo(f2, f3);
        float f4 = (float) (d * 0.575d);
        path.lineTo(f4, f3);
        path.lineTo(f4, f);
        path.close();
        paint.setColor(this.colorArrow);
        canvas.drawPath(path, paint);
    }
}
