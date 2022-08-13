// $Id: zipbrute.java,v 1.1 2001/05/08 13:35:23 thomas Exp $

import java.io.*;

public class zipbrute {
    FileInputStream file;
    
    public static void main(String[]args) {
	System.out.println("Zipbrute $Id: zipbrute.java,v 1.1 2001/05/08 13:35:23 thomas Exp $\n");
	try {
	    zipbrute zb = new zipbrute("t/foo5.zip");
	} catch(IOException e) {
	    System.out.println(e.toString());
	}
    }
    public zipbrute(String fn) throws IOException{
	file = new FileInputStream(fn);
    }
}
