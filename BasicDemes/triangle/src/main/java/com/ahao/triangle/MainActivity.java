package com.ahao.triangle;

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
        System.loadLibrary("triangle");
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
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        boolean processed = super.onKeyDown(keyCode, event);

        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed();
            processed = true;
        }

        return processed;
    }

    /**
     * 处理返回键按下事件
     */
    @Override
    @SuppressWarnings("deprecation")
    public void onBackPressed() {
        super.onBackPressed();
        System.gc();
        System.exit(0);
    }
}