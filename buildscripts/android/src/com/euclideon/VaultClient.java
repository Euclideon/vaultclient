
package com.euclideon;

import android.os.Bundle;
import android.util.*;

import org.libsdl.app.SDLActivity;

public class VaultClient extends SDLActivity
{
  @Override
  protected String[] getLibraries() {
        return new String[] {
//            "hidapi",
            "SDL2",
            "vaultSDK",
            "vaultClient"
        };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        VaultSDK.setupJNI(this);
    }
}
