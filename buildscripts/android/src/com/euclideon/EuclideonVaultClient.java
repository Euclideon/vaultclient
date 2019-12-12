
package com.euclideon;

import android.os.Bundle;
import android.util.*;

import org.libsdl.app.SDLActivity;

public class EuclideonVaultClient extends SDLActivity
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
}
