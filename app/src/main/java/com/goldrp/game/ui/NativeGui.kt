package com.goldrp.game.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.viewbinding.ViewBinding
import com.goldrp.game.NativeGuiManager
import com.goldrp.game.NativeGuiManager.Companion.deleteFromList
import com.goldrp.game.R
import com.goldrp.game.core.Samp.activity
import com.goldrp.game.common.EasyAnimation.INSTANCE
import kotlin.reflect.KClass

abstract class NativeGui<T : ViewBinding>(
    kClass: KClass<Any>,
    ishud: Boolean,
    ischat: Boolean,
    isaddToScreen: Boolean,
    ifrootView: Boolean
) {
    private val mainRenderScreen: FrameLayout = activity.findViewById(R.id.main_fl_root)
    private val uiLayout: FrameLayout = activity.findViewById(R.id.ui_layout)
    val bindingClass: KClass<Any> = kClass
    private val chat = ischat
    private val hud = ishud
    private var addToScreen = isaddToScreen
    val rootView: FrameLayout = if (ifrootView) uiLayout else mainRenderScreen

    val inflater: LayoutInflater = LayoutInflater.from(activity)
    var hiddenChat: Int = 0
    var hiddenHud: Int = 0
    private val getViewToAdd: Lazy<View> = lazy { getBinding().root }
    private val getBinding: Lazy<ViewBinding> = lazy { bindingClass.java.getMethod("inflate", LayoutInflater::class.java, ViewGroup::class.java, java.lang.Boolean.TYPE).invoke(null ,inflater, rootView, false) as ViewBinding }
    private val getUiId: Lazy<Int> = lazy { NativeGuiManager.getUiId(this) }

    init {
        activity.runOnUiThread {
            if (addToScreen) {
                getViewToAdd().clearAnimation()
                getViewToAdd().alpha = 0.0f
                rootView.addView(getViewToAdd())
                INSTANCE.animateVisible(getViewToAdd(), true, null, null, 180)
            }
        }
    }

    abstract fun receivePacket(type: Int, data: String?)
    fun getBinding(): ViewBinding {
        return getBinding.value
    }
    private fun getViewToAdd(): View {
        return getViewToAdd.value
    }
    fun toggle(toggle: Boolean) {
        activity.runOnUiThread {
            if (toggle) {
                getViewToAdd().visibility = View.VISIBLE
            } else {
                getViewToAdd().visibility = View.GONE
            }
        }
    }
    fun add(){
        activity.runOnUiThread {
            if(getViewToAdd().parent == null)
                rootView.addView(getViewToAdd())
            addToScreen = true
        }
    }
    fun destroy() {
        activity.runOnUiThread{

            if (addToScreen) {
                if(getViewToAdd().parent != null)
                    INSTANCE.HideLayout(getViewToAdd()) {
                        rootView.removeView(getViewToAdd())
                    }
            }
            if (!hud) {
                hiddenHud--
            }
            if (!chat) {
                hiddenChat--
            }
            val companion = Companion
            var z = true
            val z2 = hiddenHud <= 0
            if (hiddenChat > 0) {
                z = false
            }
            deleteFromList(getUiId())
        }
    }
    private fun getUiId(): Int {
        return getUiId.value
    }
    companion object {}
}



