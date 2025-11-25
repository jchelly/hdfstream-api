package uk.ac.dur.cosma.hdfstream;

import java.util.Scanner;

public class StringSanitizer {

    public StringSanitizer() {}

    public static boolean isPrintableAscii(String s) {

        for (int i = 0; i < s.length(); i++) {
            int c = s.charAt(i);
            if ((c < 32) || (c > 127)) {
                return false;
            }
        }
        return true;
    }

    public static void main(String[] args) {

        Scanner reader = new Scanner(System.in);
        System.out.println("Enter a string: ");
        String s = reader.next();

        System.out.println(isPrintableAscii(s));

    }
}
