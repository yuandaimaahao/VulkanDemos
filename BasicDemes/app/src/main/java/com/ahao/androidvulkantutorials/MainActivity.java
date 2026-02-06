package com.ahao.androidvulkantutorials;

import android.annotation.SuppressLint;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;

import androidx.core.view.WindowInsetsCompat;
import androidx.core.view.WindowInsetsControllerCompat;

import com.google.androidgamesdk.GameActivity;

public class MainActivity extends GameActivity {
    
    // 静态代码块，加载本地库
    static {
        System.loadLibrary("androidvulkantutorials");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // 隐藏系统UI，提供沉浸式体验
        hideSystemUI();
    }

    /**
     * 隐藏系统UI（状态栏、导航栏等），提供全屏沉浸式体验
     */
    @SuppressLint("ObsoleteSdkInt")
    private void hideSystemUI() {
        // 在有刘海屏的设备上，将游戏内容延伸到刘海区域
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
        }

        // 获取窗口的 DecorView
        View decorView = getWindow().getDecorView();
        
        // 使用 WindowInsetsControllerCompat 来控制系统栏的显示
        WindowInsetsControllerCompat controller = new WindowInsetsControllerCompat(
                getWindow(),
                decorView
        );
        
        // 隐藏系统栏（状态栏和导航栏）
        controller.hide(WindowInsetsCompat.Type.systemBars());
        
        // 隐藏刘海区域
        controller.hide(WindowInsetsCompat.Type.displayCutout());
        
        // 设置系统栏的行为：从边缘滑动时短暂显示
        controller.setSystemBarsBehavior(
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        );
    }

    /**
     * 过滤并处理返回键事件
     * 先让原生代码处理，然后在这里进行最终处理
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean processed = super.onKeyDown(keyCode, event);
        
        // 如果是返回键，调用 onBackPressed
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed();
            processed = true;
        }
        
        return processed;
    }

    /**
     * 处理返回键按下事件
     * 执行垃圾回收并退出应用
     */
    @Override
    @SuppressWarnings("deprecation")
    public void onBackPressed() {
        // 触发垃圾回收，清理资源
        super.onBackPressed();
        System.gc();
        
        // 退出应用
        System.exit(0);
    }
}

