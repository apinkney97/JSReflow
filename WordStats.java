import java.util.*;
import java.io.*;

public class WordStats {
	public static void main(String[] args) {
		if (args.length < 1) {
			System.err.println("Error: call is \"java WordStats <filename>\"");
		}
		
		HashMap<String, Integer> hm = new HashMap<String, Integer>();

		try {
			Scanner s = new Scanner(new File(args[0]));
			while (s.hasNextLine()) {
				String[] words = s.nextLine().split(" ");
				for (String word : words) {
					if (word.length() > 0) {
						int count = hm.containsKey(word) ? hm.get(word) + 1 : 1;
						hm.put(word, count);
					}
				}
			}
		} catch (Exception e) {
			System.err.println("Something went wrong:");
			e.printStackTrace();
		}

		for (Map.Entry<String, Integer> entry : hm.entrySet()) {
			System.out.println(entry.getValue() + " " + entry.getKey());
		}
	}
}
