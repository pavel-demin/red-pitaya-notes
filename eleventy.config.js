import { EleventyHtmlBasePlugin } from "@11ty/eleventy";
import { InputPathToUrlTransformPlugin } from "@11ty/eleventy";

import postcss from "postcss";
import autoprefixer from "autoprefixer";
import cssnano from "cssnano";

import site from "./_data/site.js";

const plugins = [
  autoprefixer(),
  cssnano({
    preset: "default",
  }),
];

export default function (eleventyConfig) {
  eleventyConfig.addPassthroughCopy("img");

  eleventyConfig.addPlugin(EleventyHtmlBasePlugin);
  eleventyConfig.addPlugin(InputPathToUrlTransformPlugin);

  eleventyConfig.setTemplateFormats(["md", "css"]);

  eleventyConfig.addExtension("css", {
    outputFileExtension: "css",
    compile: async (content, path) => {
      return async () => {
        return await postcss(plugins)
          .process(content, { from: path })
          .then((result) => result.css);
      };
    },
  });

  eleventyConfig.addShortcode("canonical", (url) => site.baseurl + url);

  eleventyConfig.setLiquidOptions({
    dynamicPartials: false,
    root: ["_includes", "."],
  });
}

export const config = {
  dir: {
    input: ".",
    data: "_data",
    includes: "_includes",
    layouts: "_layouts",
    output: "_site",
  },
  htmlTemplateEngine: "liquid",
  markdownTemplateEngine: "liquid",
};
