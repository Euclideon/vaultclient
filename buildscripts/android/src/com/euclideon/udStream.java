
package com.euclideon;

import java.io.File;

import android.os.Bundle;
import android.util.*;

import org.libsdl.app.SDLActivity;

public class udStream extends SDLActivity
{
    @Override
    protected String[] getLibraries() {
        return new String[] {
//            "hidapi",
            "SDL2",
            "udSDK",
            "udStream"
        };
    }

    public Boolean IsArchitectureSupported()
    {
        File nativeLibraryDir = new File(getApplicationInfo().nativeLibraryDir);
        String[] primaryNativeLibraries = nativeLibraryDir.list();
        String[] requiredLibraries = getLibraries();

        if (primaryNativeLibraries != null) {
            for (String reqLib : requiredLibraries) {
                Boolean found = false;
                for (String lib : primaryNativeLibraries) {
                    if (lib.contains(reqLib)) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    return false;
                }
            }
        }

        return true;
    }

    @Override
    public void loadLibraries() {
        if (!IsArchitectureSupported()) {
            try {
                super.loadLibraries();
            } catch(UnsatisfiedLinkError e) {
            } catch(Exception e) {
            }
            throw new UnsatisfiedLinkError("The architecture of this device is unsupported by this application.");
        }

        super.loadLibraries();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        udSDK.setupJNI(this);
    }
}
