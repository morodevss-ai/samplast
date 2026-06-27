package com.goldrp.game

import com.goldrp.game.core.Samp.activity
import com.goldrp.game.ui.NativeGui
import com.goldrp.game.ui.utils.MainMenu
import kotlin.jvm.internal.Intrinsics
import kotlin.jvm.internal.Reflection
import kotlin.reflect.KClass

class NativeGuiManager  {
    private fun <T> getInstance(i: Int): Any? {
        val hashMap = instances
        val t: Any? = hashMap[i]
        if (t != null) {
            println(" instance ready. return")
            return t
        }
        val kClass= list[i]
        if (kClass == null) {
            println("Class not found for ID: $i")
            return null
        }
        val newInstance: Any = kClass.java.newInstance()
        hashMap[i] = newInstance
        return newInstance
    }

    private fun deleteIfExists(id: Int) {
        val obj = instances[id]
        if (obj != null) {
            (obj as NativeGui<*>).destroy()
        }
    }

    fun <T> get(id: Int): Any? {
        val t = instances[id] ?: return null
        return t
    }

    companion object {
        const val ACTION_CREATE: Int = -1
        const val ACTION_DESTROY: Int = -2
        const val ACTION_GOTOP: Int = -6
        const val ACTION_HIDE: Int = -4
        const val ACTION_ONCLOSED: Int = -5
        const val ACTION_SHOW: Int = -3
        val INSTANCE: NativeGuiManager = NativeGuiManager()
        const val UI_ID_HUD: Int = 6
        private val instances = HashMap<Int, Any?>()
        private val list: HashMap<Int, KClass<out Any>> = hashMapOf(
            Pair(0, Reflection.getOrCreateKotlinClass(MainMenu::class.java)),
        )

        @JvmStatic
        fun deleteFromList(i: Int) {
            val hashMap = instances
            if (hashMap[i] != null) {
                hashMap.remove(i)
            }
        }

        @JvmStatic
        fun getUiId(obj: Any?): Int {
            var obj2: Any?
            var num: Int = 0
            val entrySet: Set<Map.Entry<Int, Any?>> = instances.entries
            val it: Iterator<*> = entrySet.iterator()
            while (true) {
                if (!it.hasNext()) {
                    obj2 = null
                    break
                }
                obj2 = it.next()
                if (Intrinsics.areEqual((obj2 as Map.Entry<*, *>?)!!.value, obj)) {
                    break
                }
            }
            val entry = obj2 as Map.Entry<*, *>?
            if (entry == null || ((entry.key as Int).also { num = it }) == null) {
                return -999
            }
            return num
        }
        @JvmStatic
        fun receiveUiPacket(id: Int, type: Int, data: String) {
            activity.runOnUiThread {

                val nativeGui = INSTANCE.getInstance<Any>(id) as NativeGui<*>?
                if (nativeGui != null) {
                    if (type == -3) {
                        INSTANCE.deleteIfExists(id)
                    }
                    if (type == -1) {
                        nativeGui.toggle(false)
                    }
                    if (type == -2) {
                        nativeGui.toggle(true)
                    } else {
                        nativeGui.receivePacket(type, data)
                    }
                }
            }
        }

    }
}
