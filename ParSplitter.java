import java.io.*;
import java.util.*;
import javax.imageio.*;
import java.awt.image.BufferedImage;

public class ParSplitter {
	public static final String FLOAT = "__FLOAT";
	public static final String PARA = "__PARA";
	public static final String PTREE = "PARAGRAPH_TREE";
	public static final int PT_PER_IN = 72;
	public static final double PT_PER_PX = 0.75;
	
	public static void main(String[] args) {
		boolean floatMode = false;
		boolean inside = false;
		
		
		
		try {
			int p = 0;
			double[] widths = {1.5, 2, 2.5, 3, 3.5, 4, 4.5};
			
			Scanner s = new Scanner(new File(args[0]));
			String line;
			String currText = null;
			double fwd = 0;
			double fht = 0;
			
			Runtime rt = Runtime.getRuntime();
			
			
			System.out.print("var GALLEY_WIDTHS = [");
			for (int i = 0; i < widths.length; i++) {
				System.out.print(widths[i] * PT_PER_IN);
				if (i + 1 < widths.length) {
					System.out.print(",");
				}
			}
			System.out.println("];");
			
			System.out.println("var " + PTREE + " = [];");
			
			while (s.hasNextLine()) {
				line = s.nextLine().trim();
				
				if (line.length() > 0) {
					if (line.startsWith(FLOAT)) { // start of a float
						if (floatMode) {
							currText += "\\\">\");"; // end of previous float
							System.out.println(currText);
							p++;
						}
						floatMode = true;
						String fname = line.substring(FLOAT.length()).trim();
						BufferedImage bimg = ImageIO.read(new File(fname));
						fwd = bimg.getWidth() * PT_PER_PX;
						fht = bimg.getHeight()  * PT_PER_PX;
						currText = PTREE + "[" + p + "] = new Floatable(" + fwd + ", " + fht + ", \"<img style=\\\"width:100%\\\" src=\\\"" + fname + "\\\" alt=\\\"";
					} else if (line.startsWith(PARA)) {
						if (floatMode) {
							currText += "\\\">\");"; // end of previous float
							System.out.println(currText);
							p++;
						}
						floatMode = false;
					} else { // Do something with the line
						if (floatMode) {
							currText += (line + "\\n");
						} else {
							if (!inside) {
								currText = "";
								System.out.println(PTREE + "[" + p + "] = [];");
								inside = true;
							}
							line = line + " ";
							currText += line;
						}
					}
					
				} else { // empty line -- either end of paragraph or gap inside float
					if (!floatMode && inside) {
						for (int g = 0; g < widths.length; g++) {
							System.out.println(PTREE + "[" + p + "][" + g + "] = [];");
							Process proc = rt.exec("./LineBreak/LineBreak - Times-Roman.afm 12 " + widths[g]);
							OutputStream out = proc.getOutputStream();
							out.write(currText.getBytes());
							out.close();
							Scanner in = new Scanner(proc.getInputStream()); // from process's stdout
							int l = 0;
							while (in.hasNextLine()) {
								System.out.println(PTREE + "["+ p +"][" + g + "][" + (l++) + "] = " + in.nextLine() + ";"); //.replaceAll("\\\"", "\\\\\""));
							}
							
							in = new Scanner(proc.getErrorStream()); // from process's stdout
							while (in.hasNextLine()) {
								System.err.println("LineBreak Error: " + in.nextLine());
							}
							
							proc.destroy() ;
							
						}
						p++;
					}
					inside = false;
				}
			}
			s.close();
		} catch (Exception e) {
			//System.err.println("FATAL: File \"" + args[0] + "\"not found, dumbass.");
			e.printStackTrace();
			System.exit(1);
		}
	}
	
}