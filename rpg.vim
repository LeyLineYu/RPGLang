" Vim syntax file
" Adapted from Vim's JavaScript syntax file
" Language:	RPGLang

if !exists("main_syntax")
  " quit when a syntax file was already loaded
  if exists("b:current_syntax")
    finish
  endif
  let main_syntax = 'rpg'
elseif exists("b:current_syntax") && b:current_syntax == "rpg"
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn match   rpgLangNumber       "0[IVXCLDM]*"

syn match rpgLangClass        "#warlock" 
syn match rpgLangClass        "#warrior"
syn match rpgLangClass        "#priest"
syn match rpgLangClass        "#mage"
syn keyword rpgLangConditional	if else
syn keyword rpgLangRepeat	while until
syn keyword rpgLangBranch	rollback skip
syn keyword rpgLangOperator	shadow unite shatter empower hit mirror or not and worthy
syn match rpgLangOperator "push>"
syn match rpgLangOperator "<push"
syn match rpgLangOperator "duel!>"
syn match rpgLangOperator "duel!<"
syn match rpgLangOperator "duel!<>"
syn keyword rpgLangType	        void prim frac loc primordial fractured location
syn keyword rpgLangStatement	complete

" DOESN'T WORK RN 
" syn match   rpgLangIdentifier   "\%(0;(){}\)\%(;(){}\)*" 

syn match rpgLangBraces	   "[{}]"
syn match rpgLangParens    "[\(\)]"

syntax region  rpgLangComment   start=+note+ end=/$/ extend keepend

if main_syntax == "rpg"
  syn sync fromstart
  syn sync maxlines=100
endif

" Define the default highlighting.
" Only when an item doesn't have highlighting yet
hi def link rpgLangComment              Comment
hi def link rpgLangNumber               Number
hi def link rpgLangConditional		Conditional
hi def link rpgLangRepeat		Repeat
hi def link rpgLangBranch		Conditional
hi def link rpgLangType			Type
hi def link rpgLangOperator             Operator
hi def link rpgLangStatement		Statement
hi def link rpgLangBraces		Function
"hi def link rpgLangIdentifier           Identifier
hi def link rpgLangClass                Statement


let b:current_syntax = "rpg"
if main_syntax == 'rpg'
  unlet main_syntax
endif
let &cpo = s:cpo_save
unlet s:cpo_save

" vim: ts=8
