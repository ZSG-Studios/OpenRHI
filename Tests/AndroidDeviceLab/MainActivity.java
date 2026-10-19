package com.openrhi.nativelab;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Color;
import android.graphics.Typeface;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

public final class MainActivity extends Activity {
    static { System.loadLibrary("OpenRHINativeLab"); }
    private static native String runNative(String files, String libraries);

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setPadding(28, 68, 28, 30);
        content.setBackgroundColor(Color.rgb(14, 19, 27));
        TextView title = new TextView(this);
        title.setText("OpenRHI Native Lab");
        title.setTextColor(Color.WHITE);
        title.setTextSize(25);
        content.addView(title);
        TextView device = new TextView(this);
        device.setText(android.os.Build.MODEL + "  |  Android " + android.os.Build.VERSION.RELEASE + "\nNative Vulkan device tests\n");
        device.setTextColor(Color.rgb(153, 182, 209));
        device.setTextSize(15);
        content.addView(device);
        ScrollView scroll = new ScrollView(this);
        TextView results = new TextView(this);
        results.setTextColor(Color.rgb(214, 232, 219));
        results.setTextSize(15);
        results.setTypeface(Typeface.MONOSPACE);
        results.setText("Running on your GPU…\n\nResults are saved on this device.");
        scroll.addView(results);
        content.addView(scroll, new LinearLayout.LayoutParams(-1, 0, 1));
        setContentView(content);
        new Thread(() -> {
            String report;
            try {
                for (String name : getAssets().list("shaders")) {
                    try (InputStream input = getAssets().open("shaders/" + name);
                         FileOutputStream output = new FileOutputStream(new File(getFilesDir(), name))) {
                        byte[] bytes = new byte[16384];
                        int count;
                        while ((count = input.read(bytes)) > 0) output.write(bytes, 0, count);
                    }
                }
                report = runNative(getFilesDir().getAbsolutePath(), getApplicationInfo().nativeLibraryDir);
            } catch (Throwable error) { report = "FAIL: " + error.toString(); }
            try (FileOutputStream output = new FileOutputStream(new File(getFilesDir(), "results.txt"))) {
                output.write(report.getBytes(StandardCharsets.UTF_8));
            } catch (Exception ignored) { }
            final String completed = report;
            runOnUiThread(() -> results.setText(completed));
        }, "OpenRHI-tests").start();
    }
}
