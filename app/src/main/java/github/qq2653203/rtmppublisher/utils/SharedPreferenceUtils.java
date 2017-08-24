package github.qq2653203.rtmppublisher.utils;

import android.content.Context;
import android.content.SharedPreferences;

public class SharedPreferenceUtils {

    public static String CONFIG = "config";
    public static String OBJECT = "object";
    private static SharedPreferences sharedPreferences;

    public static void saveStringData(Context context, String key, String value) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        sharedPreferences.edit().putString(key, value).apply();
    }

    public static String getStringData(Context context, String key, String defaultValue) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        return sharedPreferences.getString(key, defaultValue);

    }

    public static void saveBooleanData(Context context, String key, boolean value) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        sharedPreferences.edit().putBoolean(key, value).apply();

    }

    public static boolean getBooleanData(Context context, String key, boolean defaultValue) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        return sharedPreferences.getBoolean(key, defaultValue);

    }

    public static void saveIntData(Context context, String key, int value) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        sharedPreferences.edit().putInt(key, value).apply();

    }

    public static int getIntData(Context context, String key, int defaultValue) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        return sharedPreferences.getInt(key, defaultValue);

    }

    public static void saveLongData(Context context, String key, long value) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        sharedPreferences.edit().putLong(key, value).apply();

    }

    public static long getLongData(Context context, String key, long defaultValue) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        return sharedPreferences.getLong(CONFIG, defaultValue);

    }

    public static void saveFloatData(Context context, String key, float value) {

        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }

        sharedPreferences.edit().putFloat(key, value).apply();

    }

    public static float getFloatData(Context context, String key, float defaultValue) {
        if (sharedPreferences == null) {
            sharedPreferences = context.getSharedPreferences(CONFIG, Context.MODE_PRIVATE);
        }
        return sharedPreferences.getFloat(key, defaultValue);
    }


}
