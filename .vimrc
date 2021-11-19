let g:clang_format#auto_format = 1

autocmd FileType c,cpp,objc nnoremap <buffer><Leader>cf :<C-u>ClangFormat<CR>
autocmd FileType c,cpp,objc vnoremap <buffer><Leader>cf :ClangFormat<CR>

nmap <Leader>C :ClangFormatAutoToggle<CR>
