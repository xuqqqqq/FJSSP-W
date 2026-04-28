# GECCO-style paper draft

This folder contains an anonymous ACM SigConf/GECCO-style LaTeX draft for the FJSSP-W work.

Main files:

- `gecco_fjssp_w_awls_draft.tex`: paper draft.
- `fjssp_w_awls_draft_zh.tex`: Chinese-readable technical draft.
- `references.bib`: BibTeX references.

Build locally with:

```powershell
pdflatex gecco_fjssp_w_awls_draft.tex
bibtex gecco_fjssp_w_awls_draft
pdflatex gecco_fjssp_w_awls_draft.tex
pdflatex gecco_fjssp_w_awls_draft.tex
```

Build the Chinese draft with:

```powershell
xelatex fjssp_w_awls_draft_zh.tex
bibtex fjssp_w_awls_draft_zh
xelatex fjssp_w_awls_draft_zh.tex
xelatex fjssp_w_awls_draft_zh.tex
```

The draft uses the ACM `acmart` SigConf format because recent GECCO submission instructions require the ACM SigConf template and anonymized review submissions.
The Chinese file is intended for internal reading and discussion; the formal GECCO-style submission draft remains the English ACM version.
