package com.goldrp.game.ui.hud;

import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.text.Editable;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.goldrp.game.R;
import com.goldrp.game.core.Samp;
import com.goldrp.game.common.views.KeyboardInputTextView;
import com.joom.paranoid.Obfuscate;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

@Obfuscate
public class Keyboard {

    public interface InputListener {
        void OnInputEnd(String str);
    }

    private enum KeyboardLanguage {
        EN,
        RU,
        NUM
    }

    private enum KeyboardMode {
        LETTERS,
        SYMBOLS
    }

    private enum KeyType {
        CHAR,
        SHIFT,
        BACKSPACE,
        SPACE,
        ENTER,
        LANG,
        SYMBOLS
    }

    private static class KeySpec {
        final String label;
        final KeyType type;
        final float weight;

        KeySpec(String label, KeyType type, float weight) {
            this.label = label;
            this.type = type;
            this.weight = weight;
        }
    }

    private static final String[] EN_ROW1 = {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p"};
    private static final String[] EN_ROW2 = {"a", "s", "d", "f", "g", "h", "j", "k", "l"};
    private static final String[] EN_ROW3 = {"z", "x", "c", "v", "b", "n", "m"};

    private static final String[] RU_ROW1 = {"й", "ц", "у", "к", "е", "н", "г", "ш", "щ", "з", "х", "ъ"};
    private static final String[] RU_ROW2 = {"ф", "ы", "в", "а", "п", "р", "о", "л", "д", "ж", "э"};
    private static final String[] RU_ROW3 = {"я", "ч", "с", "м", "и", "т", "ь", "б", "ю"};

    private static final String[] RU_ROW1_UPPER = {"Й", "Ц", "У", "К", "Е", "Н", "Г", "Ш", "Щ", "З", "Х", "Ъ"};
    private static final String[] RU_ROW2_UPPER = {"Ф", "Ы", "В", "А", "П", "Р", "О", "Л", "Д", "Ж", "Э"};
    private static final String[] RU_ROW3_UPPER = {"Я", "Ч", "С", "М", "И", "Т", "Ь", "Б", "Ю"};

    private static final String[] NUM_ROW1 = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    private static final String[] NUM_ROW2 = {"@", "#", "$", "%", "\"", "*", "(", ")", "-", "_"};
    private static final String[] NUM_ROW3 = {".", ":", ";", "+", "=", "<", ">", "[", "]"};

    private static final String[] SYM_ROW1 = {"!", "?", "/", "\\", ":", ";", "'", "\"", ".", ","};
    private static final String[] SYM_ROW2 = {"@", "#", "$", "%", "&", "*", "-", "+", "(", ")"};

    private static final int KEYBOARD_ROWS = 3;

    private ConstraintLayout mKeyboardLayout = null;
    private FrameLayout mVisibleZone = null;
    private LinearLayout mContentLayout = null;
    private KeyboardInputTextView mContentText = null;
    private LinearLayout mContentButtonsLines = null;
    private LinearLayout mContentButtonsList = null;
    private LinearLayout mContentButtonsListLineUp = null;
    private LinearLayout mContentButtonsListLineMiddle = null;
    private LinearLayout mContentButtonsListLineDown = null;
    private LinearLayout mContentButtonsContainerFunctionSymbols = null;
    private ImageView mButtonHistoryUP = null;
    private ImageView mButtonHistoryDown = null;
    private ImageView mContentButtonSymbolShift = null;
    private ImageView mContentButtonSymbolBackspace = null;
    private Button mContentButtonSymbolSpec1 = null;
    private Button mContentButtonSymbolSpec2 = null;
    private Button mContentButtonSymbolSpecLang = null;
    private ImageView mContentButtonSymbolSpace = null;
    private Button mContentButtonSymbolSpec3 = null;
    private Button mContentButtonSymbolSpec4 = null;
    private ImageView mContentButtonSymbolEnter = null;

    private EditText mExternalTarget = null;

    private Activity mContext = null;
    private boolean mIsShowing = false;

    private String mSavedInput = null;

    private final int mMaxHistory = 20;
    private ArrayList<String> mInputHistory = null;
    private int mCurrentHistoryMessage = 0;

    private Runnable mAnimTask = null;
    private int mInputGapPx = 0;
    private int mKeyboardInsetPx = 0;
    private boolean mForceFullWidth = false;
    private boolean mAnchorToBottom = false;
    private boolean mHideInputRow = false;
    private float mKeyboardHeightRatio = 0.5f;

    private KeyboardLanguage mLanguage = KeyboardLanguage.RU;
    private KeyboardMode mMode = KeyboardMode.LETTERS;
    private boolean mShift = false;
    private int mKeyGapPx = 0;
    private int mKeyHeightPx = 0;
    private float mKeyTextSizePx = 0.0f;
    private boolean mSymbolsMode = false;

    private Typeface mDefaultTypeface;

    public boolean IsShowing() {
        return mIsShowing;
    }

    public Keyboard(Activity act) {
        mContext = act;

        mDefaultTypeface = Typeface.create("sans-serif", Typeface.BOLD);

        ConstraintLayout layout = (ConstraintLayout) act.getLayoutInflater().inflate(R.layout.keyboard, null);
        mContext.addContentView(layout, new ConstraintLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        mKeyboardLayout = layout.findViewById(R.id.keyboardStandard);
        mVisibleZone = layout.findViewById(R.id.visibleZone);
        mContentLayout = layout.findViewById(R.id.content);
        mContentText = layout.findViewById(R.id.contentText);
        mContentButtonsLines = layout.findViewById(R.id.contentButtonsLines);
        mContentButtonsList = layout.findViewById(R.id.contentButtonsList);
        mContentButtonsListLineUp = layout.findViewById(R.id.contentButtonsListLineUp);
        mContentButtonsListLineMiddle = layout.findViewById(R.id.contentButtonsListLineMiddle);
        mContentButtonsListLineDown = layout.findViewById(R.id.contentButtonsListLineDown);
        mContentButtonsContainerFunctionSymbols = layout.findViewById(R.id.contentButtonsContainerFunctionSymbols);
        mButtonHistoryUP = layout.findViewById(R.id.buttonHistoryUP);
        mButtonHistoryDown = layout.findViewById(R.id.buttonHistoryDown);
        mContentButtonSymbolShift = layout.findViewById(R.id.contentButtonSymbolShift);
        mContentButtonSymbolBackspace = layout.findViewById(R.id.contentButtonSymbolBackspace);
        mContentButtonSymbolSpec1 = layout.findViewById(R.id.contentButtonSymbolSpec1);
        mContentButtonSymbolSpec2 = layout.findViewById(R.id.contentButtonSymbolSpec2);
        mContentButtonSymbolSpecLang = layout.findViewById(R.id.contentButtonSymbolSpecLang);
        mContentButtonSymbolSpace = layout.findViewById(R.id.contentButtonSymbolSpace);
        mContentButtonSymbolSpec3 = layout.findViewById(R.id.contentButtonSymbolSpec3);
        mContentButtonSymbolSpec4 = layout.findViewById(R.id.contentButtonSymbolSpec4);
        mContentButtonSymbolEnter = layout.findViewById(R.id.contentButtonSymbolEnter);

        mContentButtonSymbolShift.setImageResource(R.drawable.button_shiftorup);
        mContentButtonSymbolBackspace.setImageResource(R.drawable.button_return);
        mContentButtonSymbolEnter.setImageResource(R.drawable.button_send);
        mContentButtonSymbolSpace.setImageResource(R.drawable.selector_keyboard_key_default_bg);

        mInputHistory = new ArrayList<String>();

        mInputGapPx = dpToPx(2.0f);
        mKeyGapPx = dpToPx(1.0f);
        mKeyboardInsetPx = 0;

        try {
            mContentText.setShowSoftInputOnFocus(false);
        } catch (Throwable ignored) {
        }

        mVisibleZone.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN && mIsShowing) {
                    if (mKeyboardLayout != null) {
                        int[] loc = new int[2];
                        mKeyboardLayout.getLocationOnScreen(loc);
                        float x = event.getRawX();
                        float y = event.getRawY();
                        boolean inside =
                                x >= loc[0] &&
                                        x <= (loc[0] + mKeyboardLayout.getWidth()) &&
                                        y >= loc[1] &&
                                        y <= (loc[1] + mKeyboardLayout.getHeight());
                        if (!inside) {
                            HideInputLayout();
                            return true;
                        }
                    }
                }
                return false;
            }
        });

        mButtonHistoryUP.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                handleHistoryUp();
            }
        });

        mButtonHistoryDown.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                handleHistoryDown();
            }
        });

        mContentButtonSymbolSpec1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mMode == KeyboardMode.SYMBOLS) {
                    mSymbolsMode = !mSymbolsMode;
                    rebuildKeyboard();
                } else {
                    insertText("/");
                }
            }
        });

        mContentButtonSymbolSpec2.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                insertText(",");
            }
        });

        mContentButtonSymbolSpecLang.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                cycleLanguage();
                rebuildKeyboard();
            }
        });

        mContentButtonSymbolSpace.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                insertText(" ");
            }
        });

        mContentButtonSymbolSpec3.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                insertText("?");
            }
        });

        mContentButtonSymbolSpec4.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                insertText("!");
            }
        });

        mContentButtonSymbolShift.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                handleShift();
            }
        });

        mContentButtonSymbolBackspace.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                deletePreviousChar();
            }
        });

        mContentButtonSymbolEnter.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                handleEnter();
            }
        });

        mContentText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int i, KeyEvent keyEvent) {
                if (i == EditorInfo.IME_ACTION_DONE || i == EditorInfo.IME_ACTION_NEXT) {
                    handleEnter();
                }
                return false;
            }
        });

        rebuildKeyboard();
        HideInputLayout();
    }

    private void handleHistoryUp() {
        mCurrentHistoryMessage--;
        if (mCurrentHistoryMessage < 0) {
            mCurrentHistoryMessage = 0;
        }
        if (mCurrentHistoryMessage <= 0) {
            if (mExternalTarget != null) {
                mExternalTarget.setText("");
            } else {
                mContentText.setText("");
            }
            return;
        }

        String historyText = mInputHistory.get(mCurrentHistoryMessage - 1);
        if (mExternalTarget != null) {
            mExternalTarget.setText(historyText);
            mExternalTarget.setSelection(historyText.length());
        } else {
            mContentText.setText(historyText);
            mContentText.setSelection(historyText.length());
        }
    }

    private void handleHistoryDown() {
        mCurrentHistoryMessage++;
        if ((mCurrentHistoryMessage - 1) >= mInputHistory.size()) {
            mCurrentHistoryMessage--;
        }
        if (mCurrentHistoryMessage <= 0) {
            return;
        }

        String historyText = mInputHistory.get(mCurrentHistoryMessage - 1);
        if (mExternalTarget != null) {
            mExternalTarget.setText(historyText);
            mExternalTarget.setSelection(historyText.length());
        } else {
            mContentText.setText(historyText);
            mContentText.setSelection(historyText.length());
        }
    }

    private void handleShift() {
        if (supportsShift()) {
            mShift = !mShift;
            rebuildKeyboard();
            updateShiftIcon();
        }
    }

    private void updateShiftIcon() {
        if (mContentButtonSymbolShift != null) {
            mContentButtonSymbolShift.setAlpha(mShift ? 1.0f : 0.5f);
        }
    }

    private void handleEnter() {
        if (mExternalTarget != null) {
            Editable editableText = mExternalTarget.getText();
            String str = editableText != null ? editableText.toString() : "";
            mExternalTarget.setText("");
            syncToContentText();
            OnInputEnd(str);
            HideInputLayout();
        } else {
            Editable editableText = mContentText.getText();
            String str = editableText != null ? editableText.toString() : "";
            mContentText.setText("");
            OnInputEnd(str);
        }
    }

    public void onHeightChanged(int height) {
        ViewGroup.LayoutParams baseParams = mKeyboardLayout.getLayoutParams();
        if (baseParams instanceof ViewGroup.MarginLayoutParams) {
            ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) baseParams;
            params.setMargins(0, 0, 0, height);
            mKeyboardLayout.setLayoutParams(params);
        }
    }

    public void ShowInputLayout() {
        mForceFullWidth = false;
        mAnchorToBottom = false;
        mHideInputRow = false;
        showBase();
    }

    public void ShowInputLayoutForDialog() {
        mForceFullWidth = true;
        mAnchorToBottom = true;
        mHideInputRow = true;
        showBase();
        applyKeyboardHeightForScreen(KEYBOARD_ROWS, false);
        rebuildKeyboard();
        mKeyboardLayout.post(() -> {
            int width = getRootWidth();
            if (width <= 0) {
                width = mContext.getResources().getDisplayMetrics().widthPixels;
            }
            applyLayoutBounds(0, 0, width, 0);
            mKeyboardLayout.setVisibility(View.VISIBLE);
        });
    }

    public void ShowInputLayoutForChatBottom() {
        mForceFullWidth = true;
        mAnchorToBottom = true;
        mHideInputRow = false;
        showBase();
        applyKeyboardHeightForScreen(KEYBOARD_ROWS, true);
        rebuildKeyboard();
        mKeyboardLayout.post(() -> {
            int width = getRootWidth();
            if (width <= 0) {
                width = mContext.getResources().getDisplayMetrics().widthPixels;
            }
            applyLayoutBounds(0, 0, width, 0);
            mKeyboardLayout.setVisibility(View.VISIBLE);
        });
    }

    private void showBase() {
        mIsShowing = true;

        mKeyboardLayout.setVisibility(View.INVISIBLE);

        if (mExternalTarget != null) {
            mExternalTarget.requestFocus();
            syncToContentText();
        } else {
            mContentText.requestFocus();
        }

        InputMethodManager imm = (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (imm != null && mContentText.getWindowToken() != null) {
            imm.hideSoftInputFromWindow(mContentText.getWindowToken(), 0);
        }

        if (mAnimTask != null) {
            mKeyboardLayout.removeCallbacks(mAnimTask);
            mAnimTask = null;
        }

        mAnimTask = new Runnable() {
            @Override
            public void run() {
                mKeyboardLayout.setVisibility(View.VISIBLE);
                if (mExternalTarget != null) {
                    mExternalTarget.requestFocus();
                } else {
                    mContentText.requestFocus();
                }
            }
        };

        mKeyboardLayout.postDelayed(mAnimTask, 60);

        mCurrentHistoryMessage = 0;

        if (mSavedInput != null) {
            if (mExternalTarget != null) {
                mExternalTarget.setText(mSavedInput);
                mExternalTarget.setSelection(mSavedInput.length());
                syncToContentText();
            } else {
                mContentText.setText(mSavedInput);
                mContentText.setSelection(mSavedInput.length());
            }
        }

        if (mExternalTarget != null) {
            CharSequence existing = mExternalTarget.getText();
            if (existing != null) {
                mContentText.setText(existing);
                mContentText.setSelection(existing.length());
            }
        }
    }

    public void ShowInputLayoutBelow(View anchor) {
        ShowInputLayout();
        if (anchor == null) {
            return;
        }
        anchor.post(() -> {
            int[] loc = new int[2];
            anchor.getLocationInWindow(loc);
            int left = loc[0];
            int bottom = loc[1] + anchor.getHeight();

            int width = anchor.getWidth();
            applyLayoutBounds(left, bottom + mInputGapPx, width, 0);
        });
    }

    public void ShowInputLayoutBelow(int[] rect, float textSizePx, float lineHeightPx) {
        ShowInputLayout();
        if (rect == null || rect.length < 4) {
            return;
        }

        int left = rect[0];
        int top = rect[1] + rect[3] + mInputGapPx;
        int width = rect[2];

        updateKeyMetrics(textSizePx, lineHeightPx);
        applyKeyboardHeightForScreen(KEYBOARD_ROWS, true);

        if (textSizePx > 0.0f) {
            mContentText.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSizePx);
        }

        rebuildKeyboard();
        applyLayoutBounds(left, top, width, 0);
    }

    private void applyLayoutBounds(int left, int top, int width, int height) {
        int rootWidth = getRootWidth();
        int adjustedLeft = mForceFullWidth ? 0 : left;
        int adjustedWidth = mForceFullWidth
                ? (rootWidth > 0 ? rootWidth : width)
                : width;

        ViewGroup.LayoutParams baseParams = mKeyboardLayout.getLayoutParams();
        if (baseParams instanceof ConstraintLayout.LayoutParams) {
            ConstraintLayout.LayoutParams lp = (ConstraintLayout.LayoutParams) baseParams;
            if (adjustedWidth > 0) {
                lp.width = adjustedWidth;
            }
            if (height > 0) {
                lp.height = height;
            } else {
                lp.height = ViewGroup.LayoutParams.MATCH_PARENT;
            }
            lp.startToStart = ConstraintLayout.LayoutParams.PARENT_ID;
            lp.endToEnd = ConstraintLayout.LayoutParams.PARENT_ID;
            if (mAnchorToBottom) {
                lp.bottomToBottom = ConstraintLayout.LayoutParams.PARENT_ID;
                lp.topToTop = ConstraintLayout.LayoutParams.UNSET;
                lp.topMargin = 0;
                lp.bottomMargin = 0;
            } else {
                lp.topToTop = ConstraintLayout.LayoutParams.PARENT_ID;
                lp.bottomToBottom = ConstraintLayout.LayoutParams.UNSET;
                lp.topMargin = top;
                lp.bottomMargin = 0;
            }
            lp.leftMargin = adjustedLeft;
            mKeyboardLayout.setLayoutParams(lp);
            mKeyboardLayout.setTranslationX(0);
            mKeyboardLayout.setTranslationY(0);
        } else {
            if (baseParams != null) {
                if (adjustedWidth > 0) {
                    baseParams.width = adjustedWidth;
                }
                if (height > 0) {
                    baseParams.height = height;
                } else {
                    baseParams.height = ViewGroup.LayoutParams.MATCH_PARENT;
                }
                mKeyboardLayout.setLayoutParams(baseParams);
            }
            mKeyboardLayout.setX(adjustedLeft);
            if (mAnchorToBottom) {
                mKeyboardLayout.post(() -> {
                    View root = mKeyboardLayout.getRootView();
                    int rootHeight = root != null ? root.getHeight() : 0;
                    int targetY = rootHeight - mKeyboardLayout.getHeight();
                    if (targetY < 0) {
                        targetY = 0;
                    }
                    mKeyboardLayout.setY(targetY);
                });
            } else {
                mKeyboardLayout.setY(top);
            }
        }
    }

    public void HideInputLayout() {
        mCurrentHistoryMessage = 0;

        if (mExternalTarget != null) {
            mSavedInput = null;
        } else if (mContentText.getEditableText() != null) {
            mSavedInput = mContentText.getEditableText().toString();
        }

        if (mAnimTask != null) {
            mKeyboardLayout.removeCallbacks(mAnimTask);
            mAnimTask = null;
        }

        if (mContext.getCurrentFocus() != null) {
            InputMethodManager inputMethodManager = (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
            inputMethodManager.hideSoftInputFromWindow(mContext.getCurrentFocus().getWindowToken(), 0);
        }
        mKeyboardLayout.setVisibility(View.GONE);

        mIsShowing = false;

        if (mContext instanceof Samp) {
            ((Samp) mContext).onKeyboardHiddenFromUi();
        }

        mExternalTarget = null;
    }

    private void OnInputEnd(String str) {
        if (mInputHistory.size() >= mMaxHistory) {
            mInputHistory.remove(mInputHistory.size() - 1);
        }
        mInputHistory.add(0, str);

        InputListener listener = (InputListener) mContext;
        listener.OnInputEnd(str);
    }

    public void setExternalTarget(EditText target) {
        mExternalTarget = target;
        if (mExternalTarget != null) {
            try {
                mExternalTarget.setShowSoftInputOnFocus(false);
            } catch (Throwable ignored) {
            }
        }
    }

    private void rebuildKeyboard() {
        if (mContentButtonsListLineUp == null || mContentButtonsListLineMiddle == null || mContentButtonsListLineDown == null) {
            return;
        }

        mContentButtonsListLineUp.removeAllViews();
        mContentButtonsListLineMiddle.removeAllViews();
        mContentButtonsListLineDown.removeAllViews();

        List<List<KeySpec>> rows = getCurrentLayout();

        if (rows.size() >= 3) {
            List<KeySpec> row1 = rows.get(0);
            float weightSum1 = 0;
            for (KeySpec spec : row1) {
                weightSum1 += spec.weight;
            }
            mContentButtonsListLineUp.setWeightSum(weightSum1);

            for (KeySpec spec : row1) {
                View keyView = createKeyView(spec);
                mContentButtonsListLineUp.addView(keyView);
            }

            List<KeySpec> row2 = rows.get(1);
            float weightSum2 = 0;
            for (KeySpec spec : row2) {
                weightSum2 += spec.weight;
            }
            mContentButtonsListLineMiddle.setWeightSum(weightSum2);

            for (KeySpec spec : row2) {
                View keyView = createKeyView(spec);
                mContentButtonsListLineMiddle.addView(keyView);
            }

            List<KeySpec> row3 = rows.get(2);
            float weightSum3 = 0;
            for (KeySpec spec : row3) {
                weightSum3 += spec.weight;
            }
            mContentButtonsListLineDown.setWeightSum(weightSum3);

            for (KeySpec spec : row3) {
                View keyView = createKeyView(spec);
                mContentButtonsListLineDown.addView(keyView);
            }

            updateSpecialButtons();
            updateShiftIcon();
        }
    }

    private View createKeyView(KeySpec spec) {
        if (spec.type == KeyType.SHIFT) {
            ImageView imageView = new ImageView(mContext);
            imageView.setImageResource(R.drawable.button_shiftorup);
            imageView.setBackgroundResource(R.drawable.selector_keyboard_key_default_background);
            imageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);

            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                    0,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    spec.weight
            );
            int halfGap = mKeyGapPx / 2;
            lp.setMargins(halfGap, halfGap, halfGap, halfGap);
            imageView.setLayoutParams(lp);

            imageView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    handleKey(spec);
                }
            });

            return imageView;
        } else if (spec.type == KeyType.BACKSPACE) {
            ImageView imageView = new ImageView(mContext);
            imageView.setImageResource(R.drawable.button_return);
            imageView.setBackgroundResource(R.drawable.selector_keyboard_key_default_background);
            imageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);

            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                    0,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    spec.weight
            );
            int halfGap = mKeyGapPx / 2;
            lp.setMargins(halfGap, halfGap, halfGap, halfGap);
            imageView.setLayoutParams(lp);

            imageView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    handleKey(spec);
                }
            });

            return imageView;
        } else {
            Button button = new Button(mContext);
            button.setAllCaps(false);
            button.setText(spec.label);
            button.setBackgroundResource(R.drawable.selector_keyboard_key_default_background);
            button.setTextSize(TypedValue.COMPLEX_UNIT_PX, mKeyTextSizePx > 0 ? mKeyTextSizePx : spToPx(20.0f));
            button.setTextColor(mContext.getResources().getColor(android.R.color.white));
            button.setTypeface(mDefaultTypeface);
            button.setIncludeFontPadding(false);

            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
                    0,
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    spec.weight
            );
            int halfGap = mKeyGapPx / 2;
            lp.setMargins(halfGap, halfGap, halfGap, halfGap);
            button.setLayoutParams(lp);

            button.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    handleKey(spec);
                }
            });

            return button;
        }
    }

    private void updateSpecialButtons() {
        if (mContentButtonSymbolSpecLang != null) {
            switch (mLanguage) {
                case RU:
                    mContentButtonSymbolSpecLang.setText("lang");
                    break;
                case EN:
                    mContentButtonSymbolSpecLang.setText("lang");
                    break;
                case NUM:
                    mContentButtonSymbolSpecLang.setText("lang");
                    break;
            }
            mContentButtonSymbolSpecLang.setTypeface(mDefaultTypeface);
        }

        if (mContentButtonSymbolSpec1 != null) {
            if (mMode == KeyboardMode.SYMBOLS) {
                mContentButtonSymbolSpec1.setText(mSymbolsMode ? "123" : "ABC");
            } else {
                mContentButtonSymbolSpec1.setText("/");
            }
            mContentButtonSymbolSpec1.setTypeface(mDefaultTypeface);
        }

        if (mContentButtonSymbolSpec2 != null) {
            mContentButtonSymbolSpec2.setTypeface(mDefaultTypeface);
        }

        if (mContentButtonSymbolSpec3 != null) {
            mContentButtonSymbolSpec3.setTypeface(mDefaultTypeface);
        }

        if (mContentButtonSymbolSpec4 != null) {
            mContentButtonSymbolSpec4.setTypeface(mDefaultTypeface);
        }
    }

    private void handleKey(KeySpec spec) {
        switch (spec.type) {
            case CHAR:
                insertText(spec.label);
                if (mShift) {
                    mShift = false;
                    rebuildKeyboard();
                }
                break;
            case SHIFT:
                if (supportsShift()) {
                    mShift = !mShift;
                    rebuildKeyboard();
                }
                break;
            case BACKSPACE:
                deletePreviousChar();
                break;
            case SPACE:
                insertText(" ");
                break;
            case ENTER:
                handleEnter();
                break;
            case LANG:
                cycleLanguage();
                rebuildKeyboard();
                break;
            case SYMBOLS:
                toggleSymbols();
                rebuildKeyboard();
                break;
        }
    }

    private void insertText(String text) {
        if (text == null || text.isEmpty()) {
            return;
        }

        if (mExternalTarget != null) {
            Editable editable = mExternalTarget.getText();
            if (editable == null) {
                return;
            }
            int start = Math.max(mExternalTarget.getSelectionStart(), 0);
            int end = Math.max(mExternalTarget.getSelectionEnd(), 0);
            int min = Math.min(start, end);
            int max = Math.max(start, end);
            editable.replace(min, max, text);
            int newPos = min + text.length();
            mExternalTarget.setSelection(newPos);
            syncToContentText();
        } else {
            Editable editable = mContentText.getText();
            if (editable == null) {
                return;
            }
            int start = Math.max(mContentText.getSelectionStart(), 0);
            int end = Math.max(mContentText.getSelectionEnd(), 0);
            int min = Math.min(start, end);
            int max = Math.max(start, end);
            editable.replace(min, max, text);
            int newPos = min + text.length();
            mContentText.setSelection(newPos);
        }
    }

    private void deletePreviousChar() {
        if (mExternalTarget != null) {
            Editable editable = mExternalTarget.getText();
            if (editable == null) {
                return;
            }
            int start = mExternalTarget.getSelectionStart();
            int end = mExternalTarget.getSelectionEnd();
            if (start < 0 || end < 0) {
                return;
            }
            if (start != end) {
                editable.delete(start, end);
                syncToContentText();
                return;
            }
            if (start == 0) {
                return;
            }
            int deleteFrom = Character.offsetByCodePoints(editable, start, -1);
            editable.delete(deleteFrom, start);
            syncToContentText();
        } else {
            Editable editable = mContentText.getText();
            if (editable == null) {
                return;
            }
            int start = mContentText.getSelectionStart();
            int end = mContentText.getSelectionEnd();
            if (start < 0 || end < 0) {
                return;
            }
            if (start != end) {
                editable.delete(start, end);
                return;
            }
            if (start == 0) {
                return;
            }
            int deleteFrom = Character.offsetByCodePoints(editable, start, -1);
            editable.delete(deleteFrom, start);
        }
    }

    private void syncToContentText() {
        if (mExternalTarget == null) {
            return;
        }
        Editable source = mExternalTarget.getText();
        if (source == null) {
            mContentText.setText("");
            return;
        }
        String text = source.toString();
        mContentText.setText(text);
        mContentText.setSelection(text.length());
    }

    private void cycleLanguage() {
        switch (mLanguage) {
            case EN:
                mLanguage = KeyboardLanguage.RU;
                mMode = KeyboardMode.LETTERS;
                break;
            case RU:
                mLanguage = KeyboardLanguage.NUM;
                mMode = KeyboardMode.LETTERS;
                break;
            case NUM:
            default:
                mLanguage = KeyboardLanguage.EN;
                mMode = KeyboardMode.LETTERS;
                break;
        }
        mShift = false;
        mSymbolsMode = false;
        rebuildKeyboard();
    }

    private void toggleSymbols() {
        if (mMode == KeyboardMode.SYMBOLS) {
            mMode = KeyboardMode.LETTERS;
        } else {
            mMode = KeyboardMode.SYMBOLS;
        }
        mShift = false;
        mSymbolsMode = false;
        rebuildKeyboard();
    }

    private boolean supportsShift() {
        return mMode == KeyboardMode.LETTERS &&
                (mLanguage == KeyboardLanguage.EN || mLanguage == KeyboardLanguage.RU);
    }

    private List<List<KeySpec>> getCurrentLayout() {
        if (mLanguage == KeyboardLanguage.NUM) {
            return buildNumberLayout();
        }

        if (mMode == KeyboardMode.SYMBOLS) {
            return buildSymbolsLayout();
        }

        switch (mLanguage) {
            case RU:
                return buildRussianLayout();
            case EN:
            default:
                return buildLetterLayout(EN_ROW1, EN_ROW2, EN_ROW3, true, Locale.ENGLISH);
        }
    }

    private List<List<KeySpec>> buildRussianLayout() {
        List<List<KeySpec>> rows = new ArrayList<>();

        List<KeySpec> firstRow = new ArrayList<>();
        String[] row1 = mShift ? RU_ROW1_UPPER : RU_ROW1;
        for (String key : row1) {
            firstRow.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        rows.add(firstRow);

        List<KeySpec> secondRow = new ArrayList<>();
        String[] row2 = mShift ? RU_ROW2_UPPER : RU_ROW2;
        for (String key : row2) {
            secondRow.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        rows.add(secondRow);

        List<KeySpec> thirdRow = new ArrayList<>();
        thirdRow.add(new KeySpec("", KeyType.SHIFT, 1.4f));
        String[] row3 = mShift ? RU_ROW3_UPPER : RU_ROW3;
        for (String key : row3) {
            thirdRow.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        thirdRow.add(new KeySpec("", KeyType.BACKSPACE, 1.4f));
        rows.add(thirdRow);

        return rows;
    }

    private List<List<KeySpec>> buildNumberLayout() {
        List<List<KeySpec>> rows = new ArrayList<>();

        List<KeySpec> row1 = new ArrayList<>();
        for (String key : NUM_ROW1) {
            row1.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        rows.add(row1);

        List<KeySpec> row2 = new ArrayList<>();
        for (String key : NUM_ROW2) {
            row2.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        rows.add(row2);

        List<KeySpec> row3 = new ArrayList<>();
        for (String key : NUM_ROW3) {
            row3.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        row3.add(new KeySpec("", KeyType.BACKSPACE, 1.4f));
        rows.add(row3);

        return rows;
    }

    private List<List<KeySpec>> buildSymbolsLayout() {
        List<List<KeySpec>> rows = new ArrayList<>();

        if (mSymbolsMode) {
            rows.add(buildCharRow(SYM_ROW1));
            rows.add(buildCharRow(SYM_ROW2));

            List<KeySpec> row3 = new ArrayList<>();
            row3.add(new KeySpec("123", KeyType.SYMBOLS, 1.4f));
            for (int i = 0; i < 4; i++) {
                row3.add(new KeySpec("", KeyType.CHAR, 1.0f));
            }
            row3.add(new KeySpec("", KeyType.BACKSPACE, 1.4f));
            rows.add(row3);
        } else {
            rows.add(buildCharRow(NUM_ROW1));
            rows.add(buildCharRow(NUM_ROW2));
            List<KeySpec> row3 = new ArrayList<>(buildCharRow(NUM_ROW3));
            row3.add(new KeySpec("", KeyType.BACKSPACE, 1.4f));
            rows.add(row3);
        }

        return rows;
    }

    private List<List<KeySpec>> buildLetterLayout(String[] row1, String[] row2, String[] row3, boolean addShift, Locale locale) {
        List<List<KeySpec>> rows = new ArrayList<>();

        List<KeySpec> firstRow = new ArrayList<>();
        for (String key : applyShift(row1, locale)) {
            firstRow.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        rows.add(firstRow);

        List<KeySpec> secondRow = new ArrayList<>();
        for (String key : applyShift(row2, locale)) {
            secondRow.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        rows.add(secondRow);

        List<KeySpec> thirdRow = new ArrayList<>();
        if (addShift) {
            thirdRow.add(new KeySpec("", KeyType.SHIFT, 1.4f));
        }
        for (String key : applyShift(row3, locale)) {
            thirdRow.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        thirdRow.add(new KeySpec("", KeyType.BACKSPACE, 1.4f));
        rows.add(thirdRow);

        return rows;
    }

    private List<KeySpec> buildCharRow(String[] row) {
        List<KeySpec> keys = new ArrayList<>();
        for (String key : row) {
            keys.add(new KeySpec(key, KeyType.CHAR, 1.0f));
        }
        return keys;
    }

    private String[] applyShift(String[] row, Locale locale) {
        if (!mShift || locale == null) {
            return row;
        }
        String[] upper = new String[row.length];
        for (int i = 0; i < row.length; i++) {
            upper[i] = row[i].toUpperCase(locale);
        }
        return upper;
    }

    private void updateKeyMetrics(float textSizePx, float lineHeightPx) {
        if (textSizePx > 0.0f) {
            mKeyTextSizePx = textSizePx;
        }
        int baseLine = Math.round(lineHeightPx > 0.0f ? lineHeightPx : (mKeyTextSizePx > 0.0f ? mKeyTextSizePx : spToPx(20.0f)));
        int extra = dpToPx(8.0f);
        mKeyHeightPx = baseLine + extra;
    }

    private int getRootWidth() {
        View root = mKeyboardLayout.getRootView();
        return root != null ? root.getWidth() : 0;
    }

    private int getRootHeight() {
        View root = mKeyboardLayout.getRootView();
        return root != null ? root.getHeight() : 0;
    }

    private void applyKeyboardHeightForScreen(int rows, boolean includeInputRow) {
        if (rows <= 0) {
            return;
        }
        int smallestWidthDp = mContext.getResources().getConfiguration().smallestScreenWidthDp;
        if (smallestWidthDp > 0 && smallestWidthDp < 600) {
            return;
        }
        int screenHeight = getRootHeight();
        if (screenHeight <= 0) {
            screenHeight = mContext.getResources().getDisplayMetrics().heightPixels;
        }
        if (screenHeight <= 0) {
            return;
        }
        int target = Math.round(screenHeight * mKeyboardHeightRatio);
        int rowCount = rows + (includeInputRow ? 1 : 0);
        int gaps = mKeyGapPx * (rows + 1);
        int available = target - gaps;
        if (available <= 0) {
            return;
        }
        int perRow = Math.max(1, available / rowCount);
        if (perRow > mKeyHeightPx) {
            mKeyHeightPx = perRow;
        }
    }

    private int dpToPx(float dp) {
        return Math.round(TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                dp,
                mContext.getResources().getDisplayMetrics()
        ));
    }

    private float spToPx(float sp) {
        return TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_SP,
                sp,
                mContext.getResources().getDisplayMetrics()
        );
    }
}