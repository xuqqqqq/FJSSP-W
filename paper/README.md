# GECCO-style paper draft

This folder contains an anonymous ACM SigConf/GECCO-style LaTeX draft for the FJSSP-W work.

Main files:

- `gecco_fjssp_w_awls_draft.tex`: paper draft.
- `references.bib`: BibTeX references.

Build locally with:

```powershell
pdflatex gecco_fjssp_w_awls_draft.tex
bibtex gecco_fjssp_w_awls_draft
pdflatex gecco_fjssp_w_awls_draft.tex
pdflatex gecco_fjssp_w_awls_draft.tex
```

The draft uses the ACM `acmart` SigConf format because recent GECCO submission instructions require the ACM SigConf template and anonymized review submissions.
