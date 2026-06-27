package com.goldrp.game.common.views;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.View;

public class KeyboardButtonEnterView extends View {
    int myColor = -7960438;

    public KeyboardButtonEnterView(Context context) {
        super(context);
    }

    public KeyboardButtonEnterView(Context context, AttributeSet attributeSet, int i) {
        super(context, attributeSet, i);
    }

    public KeyboardButtonEnterView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
    }

    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        Path path = new Path();
        Paint paint = new Paint();
        int width = getWidth();
        int height = getHeight();
        path.reset();
        double d = (double) width;
        float f = (float) (d * 0.3d);
        double d2 = (double) height;
        path.moveTo(f, (float) (0.3d * d2));
        path.lineTo((float) (d * 0.7d), (float) (0.5d * d2));
        path.lineTo(f, (float) (d2 * 0.7d));
        path.close();
        paint.setColor(this.myColor);
        canvas.drawPath(path, paint);
    }
}
