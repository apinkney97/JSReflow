import java.awt.image.BufferedImage;
import java.io.File;
import java.io.OutputStream;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Scanner;

import javax.imageio.ImageIO;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

public class ParSplitter {
	public static final String FLOAT = "__FLOAT";
	public static final String PARA = "__PARA";
	public static final String PTREE = "JSReflow.paragraph_tree";
	public static final int PT_PER_IN = 72;
	public static final double PT_PER_PX = 0.75;

	private String filename;

	public static void main(String[] args) {
		if (args.length < 1) {
			System.err.println("Error: call is \"java ParSplitter <filename> [width1[ width2 [...]]]\"");
			System.exit(-1);
		}

		String filename = args[0];
		double[] widths = { 1.5, 2, 2.5, 3, 3.5, 4, 4.5 };

		if (args.length > 1) {
			widths = new double[args.length - 1];
			for (int i = 0; i < widths.length; i++) {
				try {
					widths[i] = Double.parseDouble(args[i + 1]);
				} catch (NumberFormatException e) {
					System.err.println("Error parsing \"" + args[i + 1] + "\" as a double");
					System.exit(-1);
				}
			}
		}

		String output = new ParSplitter(filename).renderDocument(widths);
		System.out.println(output);
	}

	public ParSplitter(String filename) {
		this.filename = filename;
	}

	public String renderDocument(double[] widths) {
		boolean floatMode = false;
		boolean insidePara = false;
		HashMap<String, DictionaryWord> dictionary = new HashMap<String, DictionaryWord>();
		MalleableDocument md = new MalleableDocument();

		try {
			Scanner s = new Scanner(new File(filename));
			String line;
			String currText = null;
			double fwd = 0;
			double fht = 0;

			Runtime rt = Runtime.getRuntime();

			while (s.hasNextLine()) {
				line = s.nextLine().trim();

				if (line.length() > 0) {
					if (line.startsWith(FLOAT)) { // start of a float
						if (floatMode) {
							currText += "\\\">"; // end of previous float
							md.add(new Floatable(fwd, fht, currText));
						}
						floatMode = true;
						String[] bits = line.substring(FLOAT.length()).trim().split("\\s+");
						String fname = bits[0];
						BufferedImage bimg = ImageIO.read(new File(fname));
						if (bits.length == 1) {
							fwd = bimg.getWidth() * PT_PER_PX;
							fht = bimg.getHeight() * PT_PER_PX;
						} else if (bits.length == 2) {
							// we assume it's the maximum width, in points (so
							// need to scale height accordingly)
							fwd = Double.parseDouble(bits[1]);
							double scale = fwd / (bimg.getWidth() * PT_PER_PX);
							fht = bimg.getHeight() * PT_PER_PX * scale;
						} else {
							// Just use dimensions given and ignore the actual
							// image size
							fwd = Double.parseDouble(bits[1]);
							fht = Double.parseDouble(bits[2]);
						}
						currText = "<img style=\\\"width:100%; height:100%\\\" src=\\\"" + fname + "\\\" alt=\\\"";
					} else if (line.startsWith(PARA)) {
						if (floatMode) {
							currText += "\\\">"; // end of previous float
							md.add(new Floatable(fwd, fht, currText));
						}
						floatMode = false;
					} else { // Do something with the line
						if (floatMode) {
							currText += (line + "\\n");
						} else {
							if (!insidePara) {
								// Start of new paragraph
								currText = "";
								insidePara = true;
							}
							line = line + " ";
							currText += line;
						}
					}

				} else { // empty line -- either end of paragraph or gap inside
							// float
					if (!floatMode && insidePara) {

						Paragraph paragraph = new Paragraph();

						for (int g = 0; g < widths.length; g++) {
							// start of galley rendering");
							Process proc = rt.exec("./LineBreak/LineBreak - Times-Roman.afm 12 " + widths[g]);
							OutputStream out = proc.getOutputStream();
							out.write(currText.getBytes());
							out.close();
							Scanner in = new Scanner(proc.getInputStream());
							// from process's stdout

							StringBuilder sb = new StringBuilder();

							while (in.hasNextLine()) {
								sb.append(in.nextLine());
							}

							in = new Scanner(proc.getErrorStream());
							// from process's stderr
							while (in.hasNextLine()) {
								System.err.println("LineBreak Error: " + in.nextLine());
							}

							proc.destroy();

							JSONArray arr = (JSONArray) (JSONValue.parse(sb.toString()));

							if (arr != null) {
								Galley galley = new Galley();

								for (Object o : arr) {
									// For each line
									JSONArray typesetwords = (JSONArray) o;

									if (typesetwords.size() > 0) {
										// workaround for bug in C linebreaker

										TextLine textline = new TextLine();

										double offset = 0;

										for (Object p : typesetwords) {
											// For each word
											JSONObject typesetword = (JSONObject) p;

											String word = (String) typesetword.get("word");
											double width = Double.parseDouble((String) typesetword.get("width"));
											String key = word + " " + width;
											double absposition = Double.parseDouble((String) typesetword
													.get("position"));

											DictionaryWord dw = dictionary.get(key);

											if (dw == null) {
												dw = new DictionaryWord(word, width);
												dictionary.put(key, dw);
											}

											WordData worddata = new WordData(absposition - offset, dw);
											textline.add(worddata);

											offset = absposition + width;
										}
										galley.add(textline);
									}
								}
								paragraph.add(galley);
							} else {
								System.err.println("ARR WAS NULL AT\n\tg = [" + g + "]\n\tcurrtext = [" + currText
										+ "]\n\tsb.toString() = [" + sb.toString() + "]\n");
								System.err.println("Call was: ./LineBreak/LineBreak - Times-Roman.afm 12 " + widths[g]
										+ "\n");
							}
						}
						md.add(paragraph);
					}
					insidePara = false;
				}
			}
			s.close();
		} catch (Exception e) {
			// System.err.println("FATAL: File \"" + args[0] +
			// "\"not found, dumbass.");
			e.printStackTrace();
			System.exit(1);
		}

		// Move the dictionary to an ArrayList so we can sort it, and assign IDs
		ArrayList<DictionaryWord> sorteddict = new ArrayList<DictionaryWord>(dictionary.values());
		Collections.sort(sorteddict);

		for (int i = 0; i < sorteddict.size(); i++) {
			sorteddict.get(i).setKey(i);
		}

		StringBuilder sb = new StringBuilder();

		sb.append("var JSReflow = JSReflow || {};\n");

		sb.append("JSReflow.galley_widths = [");
		for (int i = 0; i < widths.length; i++) {
			sb.append(widths[i] * PT_PER_IN);
			if (i + 1 < widths.length) {
				sb.append(",");
			}
		}
		sb.append("];\n");

		sb.append(md.render());

		sb.append("\n");

		sb.append("JSReflow.dictionary = [");
		for (DictionaryWord dictword : sorteddict) {
			sb.append("[\"" + dictword.word + "\"," + new DecimalFormat().format(dictword.width) + "],");
		}
		sb.append("];\n");
		return sb.toString();
	}

	private class MalleableDocument {
		private ArrayList<ParLevelElement> paragraphs;

		public MalleableDocument() {
			paragraphs = new ArrayList<ParLevelElement>();
		}

		public void add(ParLevelElement e) {
			paragraphs.add(e);
		}

		public String render() {
			StringBuilder sb = new StringBuilder("JSReflow.paragraph_tree = [");
			for (ParLevelElement e : paragraphs) {
				sb.append(e.render());
				sb.append(",");
			}
			sb.append("];\n");
			return sb.toString();
		}
	}

	private interface ParLevelElement {
		public String render();
	}

	private class Floatable implements ParLevelElement {
		private double width;
		private double height;
		private String data;

		public Floatable(double width, double height, String data) {
			this.width = width;
			this.height = height;
			this.data = data;
		}

		@Override
		public String render() {
			return "{w:" + width + ",h:" + height + ",d:\"" + data + "\"}";
		}

	}

	private class Paragraph implements ParLevelElement {
		private ArrayList<Galley> galleys;

		public Paragraph() {
			galleys = new ArrayList<Galley>();
		}

		public void add(Galley galley) {
			galleys.add(galley);
		}

		@Override
		public String render() {
			StringBuilder sb = new StringBuilder("[");
			for (Galley g : galleys) {
				sb.append(g.render());
				sb.append(",");
			}
			sb.append("]");
			return sb.toString();
		}

	}

	private class Galley {

		private ArrayList<TextLine> lines;

		public Galley() {
			lines = new ArrayList<TextLine>();
		}

		public void add(TextLine line) {
			lines.add(line);
		}

		public String render() {
			StringBuilder sb = new StringBuilder("[");
			for (TextLine l : lines) {
				sb.append(l.render());
				sb.append(",");
			}
			sb.append("]");
			return sb.toString();
		}

	}

	private class TextLine {
		private ArrayList<WordData> words;

		public TextLine() {
			words = new ArrayList<WordData>();
		}

		public void add(WordData worddata) {
			words.add(worddata);
			worddata.word.incrementCount();
		}

		public String render() {
			StringBuilder sb = new StringBuilder("[");
			for (WordData w : words) {
				sb.append(w.render());
				sb.append(",");
			}
			sb.append("]");
			return sb.toString();
		}
	}

	private class WordData {
		private double position;
		private DictionaryWord word;

		public WordData(double position, DictionaryWord word) {
			this.position = position;
			this.word = word;
		}

		public String render() {
			return "[" + new DecimalFormat().format(position) + "," + word.key + "]";
		}
	}

	private class DictionaryWord implements Comparable<DictionaryWord> {
		private String word;
		private double width;
		private int count;
		private int key;

		public DictionaryWord(String word, double width) {
			this.word = word;
			this.width = width;
			count = 0;
		}

		public void incrementCount() {
			count++;
		}

		public void setKey(int k) {
			this.key = k;
		}

		@Override
		public int compareTo(DictionaryWord w) {
			int retval = w.count - this.count;

			if (retval == 0) {
				retval = w.word.length() - this.word.length();
			}
			return retval;
		}
	}

}
