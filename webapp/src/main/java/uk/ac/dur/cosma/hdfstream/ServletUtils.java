package uk.ac.dur.cosma.hdfstream;

import java.lang.System;

public class ServletUtils {

    public ServletUtils() {}

    // Check if the supplied header matches a given value
    public static boolean match_header(String header, String match) {
        String[] values = header.split(",");
	int i;
	for(i=0;i<values.length;i+=1) {
	    if(values[i].trim().equals(match.trim()) || values[i].trim().equals("*")) {
		return true;
	    }
	}
	return false;
    }

    // Extract range specifications from a range header
    public static int[][] extract_ranges(String header) {

	// Count ranges
	String[] values = header.substring(6).split(",");
	int i;
	int nrange = 0;
	int rangeArray[][] = new int[values.length][2];
	for(i=0;i<values.length;i+=1) {
	    String[] range = values[i].split("-");
	    String start = "";
	    String end   = "";
	    int istart = -1;
	    int iend   = -1;
	    if(range.length > 0)start = range[0];
	    if(range.length > 1)end   = range[1];
	    if(!start.equals("")) istart = Integer.parseInt(start.trim());
	    if(!end.equals(""))   iend   = Integer.parseInt(end.trim());
	    rangeArray[nrange][0] = istart;
	    rangeArray[nrange][1] = iend;
	    nrange += 1;
	}
	return rangeArray;
    }
}


