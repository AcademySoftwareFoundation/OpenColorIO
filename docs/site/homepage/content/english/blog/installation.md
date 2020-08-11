---
title: "How To Setup Meghna Hugo"
date: 2018-09-12T14:51:12+06:00
author: Mark Dinn
image_webp: images/blog/meghna.webp
image: images/blog/meghna.jpg
description : "This is meta description"
---

## Install this template by following those simple steps:

### STEP-1 : Hugo installation

Check this link below for install hugo on your computer.
[hugo install documentation](https://gohugo.io/getting-started/installing/)

### STEP-2 : Create your project

Hugo provides a `new` command to create a new website.

```
hugo new site <new_project>
```

### STEP-3 : Install the theme
Run this command
```
hugo new site meghna-hugo
```
and then go to the themes folder inside of meghna-hugo folder. You can also use this command ```cd meghna-hugo/themes``` for going to this folder.
Then run the command 
```
git clone git@github.com:themefisher/meghna-hugo.git
```

Alternatively, you can [download the theme as .zip](https://github.com/themefisher/meghna-hugo/archive/master.zip) file and extract it in the `themes` directory

After that you need to go to the `meghna-hugo/exampleSite` folder and copy or cut all the elements, and now go back to the root folder and paste it here.

open the command prompt again and run `cd ../` command for go back to the root folder.

### STEP-4 : Host locally

Launching the website locally by using the following command:

```
hugo serve
```

Go to `http://localhost:1313`

Or you can check this video documentation for installing this template:
{{< youtube 3O3qvDoVp5g >}}

### STEP-5 : Basic configuration

When building the website, you can set a theme by using `--theme` option. However, we suggest you modify the configuration file (`config.toml`) and set the theme as the default.

```toml
# Change the default theme to be use when building the site with Hugo
theme = "meghna-hugo"
```

### STEP-6 : Create your first content pages

```
hugo new blog/post-name.md
```

### STEP-7 : Build the website

When your site is ready to deploy, run the following command:

```
hugo

# You can also create a minified version by using this command:
hugo--minify

```

A `public` folder will be generated, containing all static content and assets for your website. It can now be deployed on any web server.